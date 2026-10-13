import { chromium, webkit, firefox } from 'playwright';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';
const dir = path.dirname(new URL(import.meta.url).pathname);
const server = await serve(8645);
for (const [name, type] of Object.entries({ chromium, webkit, firefox })) {
    for (const persistent of [true, false]) {
        const profile = path.join(dir, 'profiles', `${name}-probe`);
        fs.mkdirSync(profile, { recursive: true });
        const appData = path.join(dir, 'profiles', 'firefox-appdata');
        const env = name === 'firefox' ? { ...process.env, MOZ_APP_DATA: appData, MOZ_LOCAL_APP_DATA: appData } : undefined;
        const opts = { headless: true, ...(env && { env }) };
        const browser = persistent ? null : await type.launch(opts);
        const ctx = persistent ? await type.launchPersistentContext(profile, opts) : await browser.newContext();
        const page = ctx.pages()[0] || (await ctx.newPage());
        const got = new Promise((res) => page.on('console', (m) => m.text().startsWith('PROBE') && res(m.text().slice(6))));
        await page.goto('http://127.0.0.1:8645/probe.html');
        const r = await Promise.race([got, new Promise((r) => setTimeout(() => r('"no answer"'), 30000))]);
        console.log(name, persistent ? 'persistent' : 'ephemeral', r);
        fs.appendFileSync(path.join(dir, 'results', 'probe.jsonl'), JSON.stringify({ engine: name, persistent, ...JSON.parse(r) }) + '\n');
        await ctx.close();
        if (browser) await browser.close();
    }
}
server.close();
