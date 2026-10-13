import { chromium, webkit } from "playwright";
import { writeFileSync } from "node:fs";
const CH = process.env.HOME + "/Library/Caches/ms-playwright/chromium_headless_shell-1243/chrome-headless-shell-mac-arm64/chrome-headless-shell";
const browsers = { webkit: await webkit.launch(), chromium: await chromium.launch({ executablePath: CH }) };
const urls = { webkit: "http://localhost:8767/index.html", chromium: "http://127.0.0.1:8767/index.html" };
async function open(engine, name) {
  const page = await browsers[engine].newPage({ viewport: { width: 1100, height: 1400 } });
  const errors = [];
  page.on("pageerror", e => errors.push(String(e).slice(0, 160)));
  await page.goto(urls[engine]);
  await page.waitForFunction(() => document.getElementById("network").textContent.length > 0, null, { timeout: 60000 });
  await page.click("#generate");
  const key = await page.inputValue("#public").catch(() => page.evaluate(() => document.getElementById("public").value));
  return { engine, name, page, errors, key };
}
const logOf = p => p.page.evaluate(() => [...document.getElementById("log").children].reverse().map(li => [...li.children].map(s => s.textContent).join("  ")).join("\n"));
for (const [la, lb] of [["webkit", "chromium"], ["chromium", "webkit"]]) {
  const A = await open(la, "A"), B = await open(lb, "B");
  let t = Date.now();
  await A.page.click("#listen");
  await A.page.waitForFunction(() => /Listening|no Pkarr|failed/.test(document.getElementById("listen-status").textContent), null, { timeout: 90000 });
  const listenMs = Date.now() - t;
  t = Date.now();
  await B.page.fill("#target", A.key);
  await B.page.click("#connect");
  await B.page.waitForFunction(() => /Connected|did not answer|No /.test(document.getElementById("connect-status").textContent), null, { timeout: 120000 });
  const status = await B.page.textContent("#connect-status");
  const connectMs = Date.now() - t;
  let chat = "skipped";
  if (/Connected/.test(status)) {
    await B.page.fill("#text", `hello from ${lb}`); await B.page.click("#send");
    await A.page.waitForFunction(t => document.getElementById("messages").innerText.includes(t), `hello from ${lb}`, { timeout: 20000 });
    await A.page.fill("#text", `hello from ${la}`); await A.page.click("#send");
    await B.page.waitForFunction(t => document.getElementById("messages").innerText.includes(t), `hello from ${la}`, { timeout: 20000 });
    chat = "both ways";
  }
  const bLog = await logOf(B), aLog = await logOf(A);
  writeFileSync(`xb-${la}-listens-A.txt`, aLog); writeFileSync(`xb-${la}-listens-B.txt`, bLog);
  console.log(`\nlistener ${la}, visitor ${lb}: Listen ${listenMs} ms; visitor: "${status.slice(0, 60)}" ${connectMs} ms after Connect; chat ${chat}`);
  console.log("  route: " + (bLog.split("\n").find(l => /route local/.test(l)) ?? "(none)").slice(14, 200));
  console.log("  errors A: " + (A.errors[0] ?? "(none)") + " | B: " + (B.errors[0] ?? "(none)"));
  await A.page.screenshot({ path: `xb-${la}-A.png`, fullPage: true });
  await A.page.close(); await B.page.close();
}
await browsers.webkit.close(); await browsers.chromium.close();
