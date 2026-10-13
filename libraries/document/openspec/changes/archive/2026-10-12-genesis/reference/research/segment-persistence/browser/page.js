// The page: starts writer workers and relays what they report to the console, where the
// Playwright driver reads it. Modes (query string): cost, run (for reload and SIGKILL), terminate.
const q = new URLSearchParams(location.search);
const mode = q.get('mode');
const strategies = (q.get('strategies') || 'opfs-s0,opfs-s1,opfs-s2,idb-strict,idb-relaxed,idb-default').split(',');
const kind = q.get('kind') || mode;
const size = Number(q.get('size') || 1) << 20;
const log = (tag, o) => console.log(tag + ' ' + JSON.stringify(o));

function pct(xs, p) {
    const s = [...xs].sort((a, b) => a - b);
    return s[Math.min(s.length - 1, Math.floor((p / 100) * s.length))];
}

// The classification the protocol asks for, given the last generation the writer reported.
function classify(r, last) {
    if (r.status === 'fresh') return last == null ? 'consistent' : 'lost';
    if (r.status === 'invalid') return last == null ? 'consistent' : 'torn';
    if (r.replay !== 1) return 'torn';
    if (last != null && r.g < last) return 'lost';
    return 'consistent';
}

function once(w, pred, ms) {
    return new Promise((res, rej) => {
        const timer = setTimeout(() => rej(new Error('timeout')), ms);
        const h = (e) => {
            if (e.data.t === 'error') { clearTimeout(timer); w.removeEventListener('message', h); rej(new Error(e.data.message)); }
            else if (pred(e.data)) { clearTimeout(timer); w.removeEventListener('message', h); res(e.data); }
        };
        w.addEventListener('message', h);
    });
}

async function cost() {
    const sizes = (q.get('sizes') || '1,16,64').split(',').map(Number);
    const commits = Number(q.get('commits') || 200);
    for (const mb of sizes) {
        for (const s of strategies) {
            const w = new Worker('worker.js');
            try {
                w.postMessage({ mode: 'cost', strategy: s, name: `cost-${s}`, size: mb << 20, commits });
                const r = await once(w, (d) => d.t === 'cost', 600000);
                log('RESULT', {
                    strategy: s, mb, durability: r.durability,
                    median: pct(r.lat, 50), scanMedian: pct(r.scan, 50), p99: pct(r.lat, 99), max: Math.max(...r.lat),
                    bytesMedian: pct(r.bytes, 50), bytesMean: r.bytes.reduce((a, b) => a + b, 0) / r.bytes.length,
                });
            } catch (e) {
                log('RESULT', { strategy: s, mb, error: String(e) });
            }
            w.terminate();
        }
    }
    log('DONE', {});
}

function run() {
    for (const s of strategies) {
        const w = new Worker('worker.js');
        w.onmessage = (e) => log('MSG', { s, ...e.data });
        w.postMessage({ mode: 'crash', strategy: s, name: `${kind}-${s}`, size });
    }
}

async function terminateLoop(s, n, maxDelay) {
    const counts = { consistent: 0, torn: 0, lost: 0, hang: 0 };
    let last = null;
    let commitsTotal = 0;
    let hangsInRow = 0;
    for (let i = 0; i <= n; ++i) {
        const w = new Worker('worker.js');
        let sinceRecovery = 0;
        let recovered = false;
        let step = 'started';
        w.addEventListener('message', (e) => {
            if (e.data.t === 'c' && recovered) { last = e.data.g; ++sinceRecovery; ++commitsTotal; }
            if (e.data.t === 'step') step = e.data.step;
        });
        w.postMessage({ mode: 'crash', strategy: s, name: `${kind}-${s}`, size });
        let r;
        try {
            r = await once(w, (d) => d.t === 'r', 30000);
        } catch (e) {
            ++counts.hang;
            log('KILL', { s, i, error: String(e), step });
            // A wedge (every recovery timing out) ends this loop instead of burning it.
            if (++hangsInRow >= 5) { log('KILL', { s, i, wedged: true }); return { s, n: i + 1, ...counts, commitsTotal, wedgedAt: i - 4 }; }
            w.terminate();
            last = null;
            continue;
        }
        recovered = true;
        hangsInRow = 0;
        if (i > 0) {
            const c = classify(r, last);
            ++counts[c];
            if (c !== 'consistent') log('KILL', { s, i, c, r, last });
        }
        last = r.reset ? null : r.g;
        if (i === n) { w.terminate(); break; }
        await once(w, (d) => d.t === 'c', 30000);
        await new Promise((res) => setTimeout(res, Math.random() * maxDelay));
        w.terminate();
    }
    return { s, n, ...counts, commitsTotal };
}

async function terminate() {
    const n = Number(q.get('n') || 200);
    const maxDelay = Number(q.get('delay') || 200);
    const results = await Promise.all(strategies.map((s) => terminateLoop(s, n, maxDelay)));
    for (const r of results) log('TERM', r);
    log('DONE', {});
}

if (mode === 'cost') cost();
else if (mode === 'run') run();
else if (mode === 'terminate') terminate();
