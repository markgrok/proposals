// The Playwright driver: node run.mjs <cost|terminate|reload|sigkill> <chromium|webkit|firefox> [options]
import { chromium, webkit, firefox } from 'playwright';
import { execSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';

const dir = path.dirname(new URL(import.meta.url).pathname);
const [cmd, engine, ...rest] = process.argv.slice(2);
const opt = Object.fromEntries(rest.map((a) => a.split('=')));
const types = { chromium, webkit, firefox };
const port = Number(opt.port || 8642);
const base = `http://127.0.0.1:${port}/index.html`;
const profile = path.join(dir, 'profiles', `${engine}-${opt.profile || cmd}`);
const out = path.join(dir, 'results');
fs.mkdirSync(out, { recursive: true });
const record = (name, line) => fs.appendFileSync(path.join(out, `${name}.jsonl`), JSON.stringify({ ...line, profile: opt.profile || cmd }) + '\n');

function launch() {
    fs.mkdirSync(profile, { recursive: true });
    // macOS denies this shell's children access to ~/Library/Application Support/Firefox (another
    // app's data, owned by the installed Firefox): Gecko is pointed at an app-data dir of its own.
    const appData = path.join(dir, 'profiles', 'firefox-appdata');
    fs.mkdirSync(appData, { recursive: true });
    const env = engine === 'firefox' ? { ...process.env, MOZ_APP_DATA: appData, MOZ_LOCAL_APP_DATA: appData } : undefined;
    const args = opt.chromelog ? ['--enable-logging=stderr', '--v=0', '--vmodule=*indexed_db*=3,*indexeddb*=3,*quota*=1,*bucket*=1'] : [];
    return types[engine].launchPersistentContext(profile, { headless: true, args, ...(env && { env }) });
}

function lines(page, tag, fn) {
    page.on('console', (m) => {
        const t = m.text();
        if (t.startsWith(tag + ' ')) fn(JSON.parse(t.slice(tag.length + 1)));
        else if (opt.verbose) console.log('console:', t);
    });
    page.on('pageerror', (e) => console.log('pageerror:', e.message));
}

function classify(r, last) {
    if (r.status === 'fresh') return last == null ? 'consistent' : 'lost';
    if (r.status === 'invalid') return last == null ? 'consistent' : 'torn';
    if (r.replay !== 1) return 'torn';
    if (last != null && r.g < last) return 'lost';
    return 'consistent';
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const server = await serve(port);

async function cost() {
    const ctx = await launch();
    const page = ctx.pages()[0] || (await ctx.newPage());
    const done = new Promise((res) => lines(page, 'DONE', res));
    lines(page, 'RESULT', (r) => { console.log(JSON.stringify(r)); record(`cost-${engine}`, { engine, ...r }); });
    const qs = new URLSearchParams({ mode: 'cost', ...(opt.sizes && { sizes: opt.sizes }), ...(opt.commits && { commits: opt.commits }), ...(opt.strategies && { strategies: opt.strategies }) });
    await page.goto(`${base}?${qs}`);
    await done;
    await ctx.close();
}

async function terminate() {
    const ctx = await launch();
    const page = ctx.pages()[0] || (await ctx.newPage());
    const done = new Promise((res) => lines(page, 'DONE', res));
    lines(page, 'TERM', (r) => { console.log(JSON.stringify(r)); record('crash', { engine, kill: 'terminate', ...r }); });
    lines(page, 'KILL', (r) => { console.log('kill', JSON.stringify(r)); record('anomalies', { engine, kill: 'terminate', ...r }); });
    const qs = new URLSearchParams({ mode: 'terminate', n: opt.n || 200, delay: opt.delay || 200, ...(opt.strategies && { strategies: opt.strategies }) });
    await page.goto(`${base}?${qs}`, { timeout: 0 });
    await done;
    await ctx.close();
}

// One page runs every strategy's writer at once; one kill (a reload, or SIGKILL of the whole
// browser) interrupts all of them at independent points of their loops.
function tracker(page, strategies) {
    const st = Object.fromEntries(strategies.map((s) => [s, { last: null, r: null, after: 0 }]));
    lines(page, 'MSG', (m) => {
        const x = st[m.s];
        if (m.t === 'r') { x.r = m; x.after = 0; }
        else if (m.t === 'c') { x.last = m.g; ++x.after; }
        else if (m.t === 'error') console.log('worker error', m.s, m.message);
    });
    return st;
}

async function waitAll(st, timeout = 60000) {
    const t0 = Date.now();
    while (Object.values(st).some((x) => !x.r || x.after === 0)) {
        if (Date.now() - t0 > timeout) return false;
        await sleep(5);
    }
    return true;
}

function tally(counts, kill, st, prevLast, i) {
    for (const [s, x] of Object.entries(st)) {
        const c = x.r ? classify(x.r, prevLast[s]) : 'hang';
        counts[s] ??= { consistent: 0, torn: 0, lost: 0, hang: 0 };
        ++counts[s][c];
        if (c !== 'consistent') { console.log('anomaly', s, c, JSON.stringify(x.r), prevLast[s]); record('anomalies', { engine, kill, i, s, c, r: x.r, last: prevLast[s] }); }
    }
}

const strategies = (opt.strategies || 'opfs-s0,opfs-s1,opfs-s2,idb-strict,idb-relaxed,idb-default').split(',');
const qsRun = (kind) => new URLSearchParams({ mode: 'run', kind, strategies: strategies.join(',') });

async function reload() {
    let n = Number(opt.n || 200), hangsInRow = 0;
    const maxDelay = Number(opt.delay || 200);
    const ctx = await launch();
    const page = ctx.pages()[0] || (await ctx.newPage());
    let st = tracker(page, strategies);
    const counts = {};
    await page.goto(`${base}?${qsRun('reload')}`);
    for (let i = 0; i <= n; ++i) {
        if (!(await waitAll(st))) {
            console.log('timeout waiting for writers at', i);
            if (++hangsInRow >= 5) { console.log('wedged: 5 timeouts in a row, stopping at', i); record('anomalies', { engine, kill: cmd, i, wedged: true }); n = i; }
        } else hangsInRow = 0;
        if (i > 0) tally(counts, 'reload', st, prevLast, i);
        if (i >= n) break;
        await sleep(Math.random() * maxDelay);
        var prevLast = Object.fromEntries(Object.entries(st).map(([s, x]) => [s, x.last]));
        for (const x of Object.values(st)) { x.r = null; x.after = 0; }
        await page.evaluate(() => location.reload()).catch(() => {});
        // Messages the old page sent before it unloaded may still arrive: they update last.
        await sleep(0);
        if (i % 20 === 0) console.log('reload', i, JSON.stringify(counts));
    }
    for (const [s, c] of Object.entries(counts)) { console.log(s, JSON.stringify(c)); record('crash', { engine, kill: 'reload', s, n, ...c }); }
    await ctx.close();
}

function processes() {
    return execSync('ps -A -o pid=,ppid=,command=').toString().trim().split('\n').map((l) => {
        const m = l.trim().match(/^(\d+)\s+(\d+)\s+(.*)$/);
        return { pid: Number(m[1]), ppid: Number(m[2]), command: m[3] };
    });
}

// WebKit's XPC services (WebContent, Networking, GPU) are started by launchd, not by the UI process:
// ours are the ones of this build that did not exist before our launch.
const webkitBuild = path.dirname(webkit.executablePath()) + '/';
const webkitXpc = (ps) => ps.filter((p) => p.ppid === 1 && p.command.startsWith(webkitBuild)).map((p) => p.pid);

// The browser's whole tree: every descendant of this node process except ps's own shell, and for
// WebKit the XPC services that appeared since the launch.
function browserPids(before) {
    const ps = processes();
    const mine = new Set([process.pid]);
    for (let grew = true; grew;) {
        grew = false;
        for (const p of ps) if (!mine.has(p.pid) && mine.has(p.ppid) && !p.command.startsWith('ps -A')) { mine.add(p.pid); grew = true; }
    }
    mine.delete(process.pid);
    for (const p of ps) if (mine.has(p.pid) && /^\/bin\/sh -c ps/.test(p.command)) mine.delete(p.pid);
    if (engine === 'webkit') for (const pid of webkitXpc(ps)) if (!before.has(pid)) mine.add(pid);
    return [...mine];
}

// Chromium's IndexedDB files and the quota database, with sizes and inodes, and LevelDB's own logs.
function snapshot() {
    const d = path.join(profile, 'Default');
    return execSync(`ls -li "${d}/IndexedDB/"* "${d}/WebStorage" 2>&1; for f in "${d}"/IndexedDB/*/LOG*; do echo "== $f"; cat "$f"; done`).toString();
}

async function sigkill() {
    let n = Number(opt.n || 200), hangsInRow = 0;
    const maxDelay = Number(opt.delay || 200);
    const counts = {};
    let prevLast = null;
    for (let i = 0; i <= n; ++i) {
        const before = new Set(webkitXpc(processes()));
        const ctx = await launch();
        const page = ctx.pages()[0] || (await ctx.newPage());
        const st = tracker(page, strategies);
        await page.goto(`${base}?${qsRun('sigkill')}`);
        if (!(await waitAll(st))) {
            console.log('timeout waiting for writers at', i);
            if (++hangsInRow >= 5) { console.log('wedged: 5 timeouts in a row, stopping at', i); record('anomalies', { engine, kill: cmd, i, wedged: true }); n = i; }
        } else hangsInRow = 0;
        if (i > 0) tally(counts, 'sigkill', st, prevLast, i);
        if (opt.diag && i > 0) {
            const after = snapshot();
            const lostNow = Object.entries(st).some(([s, x]) => x.r && classify(x.r, prevLast[s]) !== 'consistent');
            if (lostNow) console.log('DIAG before relaunch:\n' + snap + '\nDIAG after relaunch:\n' + after);
        }
        if (i >= n) { await ctx.close(); break; }
        await sleep(Math.random() * maxDelay);
        const pids = browserPids(before);
        for (const pid of pids) { try { process.kill(pid, 'SIGKILL'); } catch {} }
        prevLast = Object.fromEntries(Object.entries(st).map(([s, x]) => [s, x.last]));
        if (opt.diag) { await sleep(300); var snap = snapshot(); }
        await ctx.close().catch(() => {});
        const alive = () => { const now = new Set(processes().map((p) => p.pid)); return pids.filter((p) => now.has(p)); };
        for (let t = 0; t < 200 && alive().length; ++t) await sleep(25);
        if (i % 10 === 0) console.log('sigkill', i, pids.length, 'pids', JSON.stringify(counts));
    }
    for (const [s, c] of Object.entries(counts)) { console.log(s, JSON.stringify(c)); record('crash', { engine, kill: 'sigkill', s, n, ...c }); }
}

try {
    await { cost, terminate, reload, sigkill }[cmd]();
} finally {
    server.close();
}
