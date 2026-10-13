// Runs wasmfs.html in each engine with a persistent profile and records its WASMFS lines.
import { chromium, webkit, firefox } from 'playwright';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';
const dir = path.dirname(new URL(import.meta.url).pathname);
const server = await serve(8644);
for (const [name, type] of Object.entries({ chromium, webkit, firefox })) {
    const profile = path.join(dir, 'profiles', `${name}-wasmfs`);
    fs.mkdirSync(profile, { recursive: true });
    const appData = path.join(dir, 'profiles', 'firefox-appdata');
    const env = name === 'firefox' ? { ...process.env, MOZ_APP_DATA: appData, MOZ_LOCAL_APP_DATA: appData } : undefined;
    const ctx = await type.launchPersistentContext(profile, { headless: true, ...(env && { env }) });
    const page = ctx.pages()[0] || (await ctx.newPage());
    const done = new Promise((res) => page.on('console', (m) => {
        const t = m.text();
        if (t.startsWith('WASMFS {')) { console.log(name, t); fs.appendFileSync(path.join(dir, 'results', 'wasmfs.jsonl'), JSON.stringify({ engine: name, ...JSON.parse(t.slice(7)) }) + '\n'); }
        else if (t.startsWith('WASMFS-DONE')) res();
        else console.log(name, 'console:', t.slice(0, 300));
    }));
    page.on('pageerror', (e) => console.log(name, 'pageerror', e.message));
    await page.goto('http://127.0.0.1:8644/wasmfs.html');
    console.log(name, 'crossOriginIsolated', await page.evaluate(() => crossOriginIsolated));
    await Promise.race([done, new Promise((r) => setTimeout(r, 600000))]);
    await ctx.close();
}
server.close();
