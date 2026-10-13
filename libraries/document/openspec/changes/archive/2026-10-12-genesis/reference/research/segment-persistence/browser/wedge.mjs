// WebKit's IndexedDB wedge: terminate writer workers mid-transaction until a recovery times out,
// then ask whether a page reload, a new page, or a relaunch of the browser on the same profile
// lets IndexedDB answer again.
import { webkit } from 'playwright';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';
const dir = path.dirname(new URL(import.meta.url).pathname);
const profile = path.join(dir, 'profiles', 'webkit-wedge2');
fs.mkdirSync(profile, { recursive: true });
const server = await serve(8750);
const base = 'http://127.0.0.1:8750/index.html';
// Opens the database and reads its header in the page itself, with a timeout.
const probe = (page) => page.evaluate(() => new Promise((res) => {
    const t = setTimeout(() => res('timeout (5 s)'), 5000);
    const r = indexedDB.open('terminate-idb-relaxed', 1);
    r.onerror = () => { clearTimeout(t); res('open error ' + r.error); };
    r.onsuccess = () => {
        const tx = r.result.transaction(['meta'], 'readonly');
        const q = tx.objectStore('meta').get('hdr');
        q.onsuccess = () => { clearTimeout(t); res('read ok, g=' + (q.result && q.result.g)); r.result.close(); };
        q.onerror = () => { clearTimeout(t); res('read error ' + q.error); };
    };
}));
let ctx = await webkit.launchPersistentContext(profile, { headless: true });
let page = ctx.pages()[0] || (await ctx.newPage());
const t0 = Date.now();
const done = new Promise((res) => page.on('console', (m) => { const t = m.text(); if (t.startsWith('TERM') || t.includes('wedged')) { console.log(t); if (t.startsWith('TERM')) res(); } }));
await page.goto(`${base}?mode=terminate&n=400&strategies=idb-relaxed`, { timeout: 0 });
await done;
console.log('loop ended after', Math.round((Date.now() - t0) / 1000), 's');
console.log('same page, now:', await probe(page));
await page.reload();
console.log('after page reload:', await probe(page));
const p2 = await ctx.newPage();
await p2.goto(base);
console.log('new page, same context:', await probe(p2));
await ctx.close();
ctx = await webkit.launchPersistentContext(profile, { headless: true });
page = ctx.pages()[0] || (await ctx.newPage());
await page.goto(base);
console.log('browser relaunched, same profile:', await probe(page));
await ctx.close();
server.close();
