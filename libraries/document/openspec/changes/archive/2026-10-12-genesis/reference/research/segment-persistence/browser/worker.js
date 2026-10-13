// A dedicated worker that owns one segment in its wasm memory and makes it durable with one
// strategy: opfs-s0, opfs-s1, opfs-s2 (FileSystemSyncAccessHandle) or idb-strict, idb-relaxed,
// idb-default (IndexedDB, the dirty pages and the header in one readwrite transaction).
//
// Messages in: {mode: 'crash', strategy, name, size} recovers, reports {t: 'r'}, then turns and
// commits forever, reporting {t: 'c', g} after each commit returns; {mode: 'cost', strategy, name,
// size, commits} starts fresh and reports {t: 'cost', lat: [...ms], bytes: [...]}.
importScripts('seg.js');

const PAGE = 4096;
const MAGIC = 0x53454731; // "SEG1"
const CMIT = 0x54494d43; // "CMIT"
const PAGE_TAG = 0x45474150; // "PAGE"
const CHECKPOINT_EVERY = 64;
const RESET_ABOVE = 1500;
let M;

const heap = () => M.HEAPU8;
const view = (ptr, len) => M.HEAPU8.subarray(ptr, ptr + len);

class Segment {
    constructor(size) {
        this.size = size;
        this.body = M._seg_alloc(size);
        this.shadow = M._seg_alloc(size);
        this.dig = M._seg_alloc(PAGE);
        this.offset = 0;
    }
    create() {
        this.offset = M._seg_create(this.body, this.size);
    }
    digest(ptr = this.body, len = this.size) {
        M._seg_digest(ptr, len, this.dig);
        return new DataView(heap().buffer, this.dig, 8).getBigUint64(0, true);
    }
    syncShadow() {
        heap().copyWithin(this.shadow, this.body, this.body + this.size);
    }
    zero(ptr) {
        heap().fill(0, ptr, ptr + this.size);
    }
}

function header(fields) {
    const b = new ArrayBuffer(PAGE);
    const d = new DataView(b);
    d.setUint32(0, MAGIC, true);
    d.setUint32(4, fields.g, true);
    d.setUint32(8, fields.size, true);
    d.setUint32(12, fields.offset, true);
    d.setBigUint64(16, fields.digest ?? 0n, true);
    return new Uint8Array(b);
}

function parseHeader(bytes) {
    const d = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (d.getUint32(0, true) !== MAGIC) return null;
    return { g: d.getUint32(4, true), size: d.getUint32(8, true), offset: d.getUint32(12, true), digest: d.getBigUint64(16, true) };
}

async function syncHandle(name) {
    const root = await navigator.storage.getDirectory();
    const fh = await root.getFileHandle(name, { create: true });
    // A terminated worker's lock is released asynchronously: retry for a while.
    for (let i = 0; ; ++i) {
        try {
            return await fh.createSyncAccessHandle();
        } catch (e) {
            if (i > 400) throw e;
            await new Promise((r) => setTimeout(r, 25));
        }
    }
}

function writeAll(ah, bytes, at) {
    const n = ah.write(bytes, { at });
    if (n !== bytes.byteLength) throw new Error(`short write ${n} of ${bytes.byteLength}`);
    return n;
}

function readAll(ah, bytes, at) {
    return ah.read(bytes, { at });
}

// S0: header at 0, image at 4096, written in place, then one flush.
class OpfsS0 {
    constructor(seg, name) { this.seg = seg; this.name = name; this.bytes = 0; }
    async open() {
        this.ah = await syncHandle(this.name);
        if (this.ah.getSize() === 0) return { status: 'fresh' };
        const hb = new Uint8Array(PAGE);
        readAll(this.ah, hb, 0);
        const h = parseHeader(hb);
        if (!h || h.size !== this.seg.size) return { status: 'invalid' };
        readAll(this.ah, view(this.seg.body, this.seg.size), PAGE);
        this.seg.offset = h.offset;
        return { status: 'ok', g: h.g };
    }
    reset() { this.ah.truncate(0); this.ah.flush(); }
    commit(g) {
        const s = this.seg;
        this.bytes = writeAll(this.ah, view(s.body, s.size), PAGE);
        this.bytes += writeAll(this.ah, header({ g, size: s.size, offset: s.offset }), 0);
        this.ah.flush();
    }
}

// S1: header A at 0, header B at 4096, slot A at 8192, slot B after it.
class OpfsS1 {
    constructor(seg, name) { this.seg = seg; this.name = name; this.active = 1; this.bytes = 0; }
    slotAt(i) { return 2 * PAGE + i * this.seg.size; }
    async open() {
        const s = this.seg;
        this.ah = await syncHandle(this.name);
        if (this.ah.getSize() === 0) return { status: 'fresh' };
        const hs = [0, 1].map((i) => {
            const b = new Uint8Array(PAGE);
            readAll(this.ah, b, i * PAGE);
            const h = parseHeader(b);
            return h && h.size === s.size ? { ...h, i } : null;
        }).filter(Boolean).sort((a, b) => b.g - a.g);
        for (const h of hs) {
            readAll(this.ah, view(s.body, s.size), this.slotAt(h.i));
            if (s.digest() === h.digest) {
                s.offset = h.offset;
                this.active = h.i;
                return { status: 'ok', g: h.g };
            }
        }
        return { status: 'invalid' };
    }
    reset() { this.ah.truncate(0); this.ah.flush(); this.active = 1; }
    commit(g) {
        const s = this.seg;
        const target = 1 - this.active;
        const t0 = performance.now();
        const digest = s.digest();
        this.scanMs = performance.now() - t0; // the CPU part: the slot's digest
        this.bytes = writeAll(this.ah, view(s.body, s.size), this.slotAt(target));
        this.ah.flush();
        this.bytes += writeAll(this.ah, header({ g, size: s.size, offset: s.offset, digest }), target * PAGE);
        this.ah.flush();
        this.active = target;
    }
}

// S2: the image file (header at 0, image at 4096) and a log of dirty pages with commit records.
class OpfsS2 {
    constructor(seg, name) { this.seg = seg; this.name = name; this.bytes = 0; this.ckptBytes = 0; }
    async open() {
        const s = this.seg;
        this.img = await syncHandle(this.name + '.img');
        this.log = await syncHandle(this.name + '.log');
        this.records = M._seg_alloc((s.size / PAGE) * (PAGE + 8) + PAGE);
        this.since = new Set();
        this.commits = 0;
        this.end = 0;
        if (this.img.getSize() === 0) return { status: 'fresh' };
        const hb = new Uint8Array(PAGE);
        readAll(this.img, hb, 0);
        const h = parseHeader(hb);
        if (!h || h.size !== s.size) return { status: 'invalid' };
        s.zero(s.body);
        readAll(this.img, view(s.body, s.size), PAGE);
        s.offset = h.offset;
        let g = h.g;
        // Replays every complete commit whose digest matches; stops at the first that does not.
        const logSize = this.log.getSize();
        const buf = M._seg_alloc(logSize + PAGE);
        readAll(this.log, view(buf, logSize), 0);
        let pos = 0, start = 0;
        const pages = [];
        const dv = () => new DataView(heap().buffer, buf, logSize);
        while (pos + 8 <= logSize) {
            const tag = dv().getUint32(pos, true);
            if (tag === PAGE_TAG && pos + 8 + PAGE <= logSize) {
                pages.push([dv().getUint32(pos + 4, true), pos + 8]);
                pos += 8 + PAGE;
            } else if (tag === CMIT && pos + 24 <= logSize) {
                const count = dv().getUint32(pos + 4, true);
                const cg = dv().getUint32(pos + 8, true);
                const digest = dv().getBigUint64(pos + 16, true);
                const got = s.digest(buf + start, pos - start);
                if (count !== pages.length || got !== digest) {
                    this.fail = { cg, count, pages: pages.length, want: String(digest), got: String(got), start, pos };
                    break;
                }
                for (const [p, at] of pages) {
                    if (p >= s.size / PAGE) break;
                    heap().copyWithin(s.body + p * PAGE, buf + at, buf + at + PAGE);
                }
                g = cg;
                pages.length = 0;
                pos += 24;
                start = pos;
            } else {
                break;
            }
        }
        M._seg_free(buf);
        return { status: 'ok', g, diag: { hg: h.g, logSize, replayed: g - h.g, tail: logSize - pos, fail: this.fail } };
    }
    reset() {
        this.img.truncate(0); this.img.flush();
        this.log.truncate(0); this.log.flush();
        this.since.clear(); this.commits = 0; this.end = 0;
    }
    // After recovery: the recovered image becomes the checkpoint and the log is emptied.
    adopt(g) {
        const s = this.seg;
        writeAll(this.img, view(s.body, s.size), PAGE);
        writeAll(this.img, header({ g, size: s.size, offset: s.offset }), 0);
        this.img.flush();
        this.log.truncate(0);
        this.log.flush();
        s.syncShadow();
        this.end = 0;
    }
    first(g) {
        // A fresh segment: the whole image is the first checkpoint.
        const s = this.seg;
        this.bytes = writeAll(this.img, view(s.body, s.size), PAGE);
        this.bytes += writeAll(this.img, header({ g, size: s.size, offset: s.offset }), 0);
        this.img.flush();
        this.log.truncate(0);
        this.log.flush();
        s.syncShadow();
    }
    commit(g) {
        const s = this.seg;
        const t0 = performance.now();
        const n = M._seg_log_records(s.body, s.shadow, s.size, this.records);
        this.scanMs = performance.now() - t0;
        const len = n * (PAGE + 8);
        const dv = new DataView(heap().buffer, this.records, len);
        for (let i = 0; i < n; ++i) this.since.add(dv.getUint32(i * (PAGE + 8) + 4, true));
        const rec = new ArrayBuffer(24);
        const r = new DataView(rec);
        r.setUint32(0, CMIT, true);
        r.setUint32(4, n, true);
        r.setUint32(8, g, true);
        r.setBigUint64(16, s.digest(this.records, len), true);
        this.bytes = len ? writeAll(this.log, view(this.records, len), this.end) : 0;
        this.bytes += writeAll(this.log, new Uint8Array(rec), this.end + len);
        this.log.flush();
        this.end += len + 24;
        this.ckptBytes = 0;
        if (++this.commits % CHECKPOINT_EVERY === 0) this.checkpoint(g);
    }
    checkpoint(g) {
        const s = this.seg;
        for (const p of this.since) this.ckptBytes += writeAll(this.img, view(s.shadow + p * PAGE, PAGE), PAGE + p * PAGE);
        this.ckptBytes += writeAll(this.img, header({ g, size: s.size, offset: s.offset }), 0);
        this.img.flush();
        this.log.truncate(0);
        this.log.flush();
        this.since.clear();
        this.end = 0;
    }
}

function req(r) {
    return new Promise((res, rej) => { r.onsuccess = () => res(r.result); r.onerror = () => rej(r.error); });
}

// IndexedDB: store 'pages' (key: page number, value: 4096 bytes) and store 'meta' (key 'hdr').
class Idb {
    constructor(seg, name, durability) {
        this.seg = seg; this.name = name; this.durability = durability; this.bytes = 0;
    }
    async open() {
        const s = this.seg;
        const r = indexedDB.open(this.name, 1);
        r.onupgradeneeded = () => {
            r.result.createObjectStore('pages');
            r.result.createObjectStore('meta');
        };
        r.onblocked = () => postMessage({ t: 'step', step: 'open blocked' });
        this.db = await req(r);
        postMessage({ t: 'step', step: 'opened' });
        this.list = M._seg_alloc(4 * (s.size / PAGE));
        const tx = this.db.transaction(['pages', 'meta'], 'readonly');
        const h = await req(tx.objectStore('meta').get('hdr'));
        if (!h) return { status: 'fresh' };
        if (h.size !== s.size) return { status: 'invalid' };
        postMessage({ t: 'step', step: 'header read' });
        const keys = await req(tx.objectStore('pages').getAllKeys());
        const values = await req(tx.objectStore('pages').getAll());
        s.zero(s.body);
        keys.forEach((p, i) => heap().set(values[i], s.body + p * PAGE));
        s.offset = h.offset;
        return { status: 'ok', g: h.g };
    }
    async reset() {
        const tx = this.db.transaction(['pages', 'meta'], 'readwrite', { durability: 'strict' });
        tx.objectStore('pages').clear();
        tx.objectStore('meta').clear();
        await new Promise((res, rej) => { tx.oncomplete = res; tx.onerror = tx.onabort = () => rej(tx.error); });
    }
    adopt() { this.seg.syncShadow(); }
    first(g) { this.seg.zero(this.seg.shadow); return this.commit(g); }
    commit(g) {
        const s = this.seg;
        const t0 = performance.now();
        const n = M._seg_dirty(s.body, s.shadow, s.size, this.list);
        this.scanMs = performance.now() - t0;
        const pages = Array.from(new Uint32Array(heap().buffer, this.list, n));
        const tx = this.durability === 'default'
            ? this.db.transaction(['pages', 'meta'], 'readwrite')
            : this.db.transaction(['pages', 'meta'], 'readwrite', { durability: this.durability });
        this.reported = tx.durability;
        const store = tx.objectStore('pages');
        // slice, not subarray: a view would make IndexedDB clone the whole wasm memory.
        for (const p of pages) store.put(heap().slice(s.body + p * PAGE, s.body + (p + 1) * PAGE), p);
        tx.objectStore('meta').put({ g, size: s.size, offset: s.offset }, 'hdr');
        this.bytes = n * PAGE + 16;
        const done = new Promise((res, rej) => { tx.oncomplete = res; tx.onerror = tx.onabort = () => rej(tx.error); });
        if (tx.commit) tx.commit();
        return done;
    }
}

function strategyOf(strategy, seg, name) {
    switch (strategy) {
        case 'opfs-s0': return new OpfsS0(seg, name);
        case 'opfs-s1': return new OpfsS1(seg, name);
        case 'opfs-s2': return new OpfsS2(seg, name);
        case 'idb-strict': return new Idb(seg, name, 'strict');
        case 'idb-relaxed': return new Idb(seg, name, 'relaxed');
        case 'idb-default': return new Idb(seg, name, 'default');
    }
    throw new Error('unknown strategy ' + strategy);
}

async function fresh(st, seg) {
    seg.create();
    if (st.first) await st.first(0); else await st.commit(0);
    return 0;
}

async function crash({ strategy, name, size }) {
    const seg = new Segment(size);
    const st = strategyOf(strategy, seg, name);
    const t0 = performance.now();
    const rec = await st.open();
    let replay = null;
    if (rec.status === 'ok') {
        const scratch = M._seg_alloc(size);
        try {
            replay = M._seg_equals_replay(seg.body, size, seg.offset, rec.g, scratch);
        } catch (e) {
            replay = -2; // a trap: the image is so wrong that checking it reads outside the wasm memory
        }
        M._seg_free(scratch);
    }
    let g;
    let reset = false;
    if (rec.status === 'ok' && replay === 1 && rec.g <= RESET_ABOVE) {
        g = rec.g;
        if (st.adopt) await st.adopt(g);
    } else {
        reset = true;
        if (rec.status !== 'fresh') await st.reset();
        g = await fresh(st, seg);
    }
    postMessage({ t: 'r', status: rec.status, g: rec.g, diag: rec.diag, replay, reset, ms: performance.now() - t0, durability: st.reported });
    if (reset) postMessage({ t: 'c', g });
    for (;;) {
        ++g;
        M._seg_turn(seg.body, size, seg.offset, g);
        await st.commit(g);
        postMessage({ t: 'c', g });
    }
}

async function cost({ strategy, name, size, commits }) {
    const seg = new Segment(size);
    const st = strategyOf(strategy, seg, name);
    await st.open();
    await st.reset?.();
    await fresh(st, seg);
    const lat = [], bytes = [], scan = [];
    for (let g = 1; g <= commits; ++g) {
        M._seg_turn(seg.body, size, seg.offset, g);
        const t = performance.now();
        await st.commit(g);
        lat.push(performance.now() - t);
        bytes.push(st.bytes + (st.ckptBytes ?? 0));
        scan.push(st.scanMs ?? 0);
    }
    postMessage({ t: 'cost', lat, bytes, scan, durability: st.reported });
}

onmessage = async (e) => {
    try {
        M ??= await createSeg();
        if (e.data.mode === 'crash') await crash(e.data);
        else if (e.data.mode === 'cost') await cost(e.data);
    } catch (err) {
        postMessage({ t: 'error', message: String(err && err.stack || err) });
    }
};
