import { chromium, webkit } from "playwright";
const CH = process.env.HOME + "/Library/Caches/ms-playwright/chromium_headless_shell-1243/chrome-headless-shell-mac-arm64/chrome-headless-shell";
for (const [name, type, opts] of [["webkit", webkit, {}], ["chromium", chromium, { executablePath: CH }]]) {
  const browser = await type.launch(opts);
  const page = await browser.newPage({ viewport: { width: 390, height: 900 } });
  const errors = [];
  page.on("pageerror", e => errors.push(String(e).slice(0, 120)));
  await page.goto("http://192.168.1.178:8770/index.html");
  await page.waitForTimeout(4000);
  const info = await page.evaluate(() => ({ banner: document.getElementById("fatal").hidden ? null : document.getElementById("fatal").textContent.slice(0, 140), disabled: [...document.querySelectorAll("button")].every(b => b.disabled) }));
  console.log(name, JSON.stringify(info), "pageerror:", errors[0] ?? "(none)");
  await page.screenshot({ path: `guard-${name}.png` });
  await browser.close();
}
