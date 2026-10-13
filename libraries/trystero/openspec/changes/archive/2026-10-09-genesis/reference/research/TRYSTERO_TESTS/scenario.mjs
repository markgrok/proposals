import { writeFileSync } from "node:fs";
const [, , url, outDir] = process.argv;
async function open(name) {
  const target = await (await fetch("http://127.0.0.1:9335/json/new?about:blank", { method: "PUT" })).json();
  const ws = new WebSocket(target.webSocketDebuggerUrl);
  let id = 0; const pending = new Map(); const errors = [];
  const send = (method, params = {}) => new Promise((resolve, reject) => { const i = ++id; pending.set(i, { resolve, reject }); ws.send(JSON.stringify({ id: i, method, params })); });
  ws.onmessage = m => { const d = JSON.parse(m.data);
    if (d.id && pending.has(d.id)) { const p = pending.get(d.id); pending.delete(d.id); d.error ? p.reject(new Error(d.error.message)) : p.resolve(d.result); }
    if (d.method === "Runtime.exceptionThrown") errors.push("EXCEPTION " + (d.params.exceptionDetails.exception?.description ?? d.params.exceptionDetails.text));
    if (d.method === "Runtime.consoleAPICalled" && ["error", "warning"].includes(d.params.type)) errors.push(`console.${d.params.type} ` + d.params.args.map(a => a.value ?? a.description).join(" ")); };
  await new Promise(r => (ws.onopen = r));
  await send("Runtime.enable"); await send("Page.enable");
  const js = async e => { const r = await send("Runtime.evaluate", { expression: e, awaitPromise: true, returnByValue: true }); if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description ?? r.exceptionDetails.text); return r.result.value; };
  const waitFor = async (e, ms = 60000) => { const end = Date.now() + ms; while (Date.now() < end) { const v = await js(e); if (v) return v; await new Promise(r => setTimeout(r, 250)); } throw new Error(`${name}: timeout waiting for ${e}`); };
  const shot = async (file, width = 1100) => { await send("Emulation.setDeviceMetricsOverride", { width, height: 900, deviceScaleFactor: 1, mobile: width < 600 }); await new Promise(r => setTimeout(r, 300)); const { data } = await send("Page.captureScreenshot", { format: "png", captureBeyondViewport: true }); writeFileSync(`${outDir}/${file}.png`, Buffer.from(data, "base64")); };
  await send("Page.addScriptToEvaluateOnNewDocument", { source: "const O = RTCPeerConnection; window.__pcs = 0; window.RTCPeerConnection = class extends O { constructor(...a) { super(...a); window.__pcs++; } }; window.__dcSends = 0; const S = RTCDataChannel.prototype.send; RTCDataChannel.prototype.send = function (...a) { window.__dcSends++; return S.apply(this, a); };" });
  await send("Page.navigate", { url });
  await waitFor("document.readyState === 'complete' && document.getElementById('network').textContent.length > 0");
  await js("document.getElementById('generate').click()");
  const key = await waitFor("document.getElementById('public').value");
  console.log(`${name} key ${key}`);
  return { name, key, js, waitFor, shot, errors, close: () => ws.close() };
}
const text = (p, id) => p.js(`document.getElementById('${id}').innerText`);
const t0 = Date.now(); const ms = () => `${((Date.now() - t0) / 1000).toFixed(1)}s`;
const count = (p, re) => p.js(`[...document.querySelectorAll('#log li')].filter(li => ${re}.test(li.textContent)).length`);
const peersOf = async p => (await text(p, "peers")).split("\n").filter(l => l.includes("(peer ")).map(l => l.split(" ")[0].slice(0, 6));
const signal = new URL(url).searchParams.get("signal");

const A = await open("A"), B = await open("B"), C = await open("C");
await A.js("document.getElementById('listen').click()");
await A.waitFor("document.getElementById('listen-status').className.includes('ok')");
const listenedAt = Date.now();
const topic = (await text(A, "listen-status")).match(/_topic ([0-9a-f]{32})/)[1];
console.log(`[${ms()}] A listening on ${topic}`);

for (const p of [B, C]) {
  const start = Date.now();
  await p.js(`document.getElementById('target').value = ${JSON.stringify(A.key)}; document.getElementById('connect').click()`);
  await p.waitFor(`document.getElementById('peers').innerText.includes(${JSON.stringify(A.key)})`);
  console.log(`[${ms()}] ${p.name} verified A in ${Date.now() - start} ms`);
}
await A.waitFor(`(t => t.includes(${JSON.stringify(B.key)}) && t.includes(${JSON.stringify(C.key)}))(document.getElementById('peers').innerText)`);
await new Promise(r => setTimeout(r, 8000));
console.log(`[${ms()}] after 8 s: A peers ${await peersOf(A)} | B peers ${await peersOf(B)} | C peers ${await peersOf(C)} | rejections B ${await count(B, /Rejected/)} C ${await count(C, /Rejected/)}`);

await B.js("document.getElementById('text').value = 'hello from B'; document.getElementById('send').click()");
await A.waitFor("document.getElementById('messages').innerText.includes('hello from B')");
await A.js("document.getElementById('text').value = 'hello to both from A'; document.getElementById('send').click()");
await B.waitFor("document.getElementById('messages').innerText.includes('hello to both from A')");
await C.waitFor("document.getElementById('messages').innerText.includes('hello to both from A')");
const cSawB = (await text(C, "messages")).includes("hello from B");
console.log(`[${ms()}] chat: A got B's message, B and C got A's; C saw B's message: ${cSawB}`);

const base0 = await B.js("window.__dcSends"); await new Promise(r => setTimeout(r, 10000)); const base = (await B.js("window.__dcSends")) - base0;
console.log(`[${ms()}] baseline: B sent ${base} data-channel messages in 10 s with only A connected`);
const E = await open("E");
await E.js(`(async () => {
  const joinRoom = ${signal ? `(await import("./ws-relay-strategy.js")).wsRelayStrategy({ steadyAnnounceIntervalMs: 5000 })` : `(await import("trystero")).joinRoom`};
  window.__impostor = joinRoom({ appId: "pkarr-p2p-demo/v1", ${signal ? `relayConfig: { urls: [${JSON.stringify(signal)}] },` : ""} }, ${JSON.stringify(topic)}, {
    onPeerHandshake: async (id, send, receive) => {
      await send({ key: ${JSON.stringify(A.key)}, nonce: "00".repeat(32), fingerprint: "00" });
      await receive();
      await send({ signature: "11".repeat(64) });
      await receive();
    },
  });
  return true;
})()`);
const pcsBefore = { B: await B.js("window.__pcs"), C: await C.js("window.__pcs") }; const dcBefore = await B.js("window.__dcSends");
console.log(`[${ms()}] impostor E joined A's topic actively, claiming A's key`);
await new Promise(r => setTimeout(r, 10000));
console.log(`[${ms()}] 10 s later: rejections B ${await count(B, /Rejected/)} C ${await count(C, /Rejected/)} A ${await count(A, /Rejected/)} | B peers ${await peersOf(B)} | C peers ${await peersOf(C)} | RTCPeerConnections created during those 10 s: B ${(await B.js('window.__pcs')) - pcsBefore.B}, C ${(await C.js('window.__pcs')) - pcsBefore.C}; B data-channel messages ${(await B.js('window.__dcSends')) - dcBefore}`);
for (const p of [A, B]) console.log(`  ${p.name} first rejection: ${await p.js("[...document.querySelectorAll('#log li')].reverse().find(li => /Rejected/.test(li.textContent))?.textContent ?? '(none)'")}`);
await E.js("window.__impostor.leave(), true");

const Dp = await open("D");
const wait = 45000 - (Date.now() - listenedAt);
if (wait > 0) await new Promise(r => setTimeout(r, wait));
const dStart = Date.now();
await Dp.js(`document.getElementById('target').value = ${JSON.stringify(A.key)}; document.getElementById('connect').click()`);
await Dp.waitFor(`document.getElementById('peers').innerText.includes(${JSON.stringify(A.key)})`, 120000);
console.log(`[${ms()}] late visitor D (${Math.round((dStart - listenedAt) / 1000)} s after A listened) verified A in ${Date.now() - dStart} ms; D peers ${await peersOf(Dp)}`);

for (const p of [A, B]) console.log(`${p.name} log:\n  ` + (await text(p, 'log')).split('\n').slice(0, 8).join('\n  '));
console.log(`B connect status: ${await text(B, 'connect-status')}`);
for (const p of [A, B, C, Dp, E]) console.log(`${p.name} page errors: ${[...new Set(p.errors)].slice(0, 3).join(" | ") || "(none)"}`);
await A.shot("A2-desktop"); await B.shot("B2-mobile", 390);
for (const p of [A, B, C, Dp, E]) p.close();
process.exit(0);
