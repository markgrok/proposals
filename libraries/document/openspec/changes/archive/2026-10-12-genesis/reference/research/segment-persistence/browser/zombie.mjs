import { chromium, webkit, firefox } from 'playwright';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';
const dir = path.dirname(new URL(import.meta.url).pathname);
const server = await serve(8643);
for (const [name, type] of Object.entries({ chromium, webkit, firefox })) {
    const profile = path.join(dir, 'profiles', `${name}-zombie`);
    fs.mkdirSync(profile, { recursive: true });
    const appData = path.join(dir, 'profiles', 'firefox-appdata');
    const env = name === 'firefox' ? { ...process.env, MOZ_APP_DATA: appData, MOZ_LOCAL_APP_DATA: appData } : undefined;
    const ctx = await type.launchPersistentContext(profile, { headless: true, ...(env && { env }) });
    const page = ctx.pages()[0] || (await ctx.newPage());
    const got = new Promise((res) => page.on('console', (m) => m.text().startsWith('ZOMBIE') && res(m.text())));
    await page.goto('http://127.0.0.1:8643/zombie.html');
    console.log(name, await Promise.race([got, new Promise((r) => setTimeout(() => r('no answer in 30 s'), 30000))]));
    await ctx.close();
}
server.close();
