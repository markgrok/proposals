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
const logOf = p => p.js("[...document.getElementById('log').children].reverse().map(li => [...li.children].map(s => s.textContent).join('  ')).join('\\n')");
const A = await open("A"), B = await open("B");
let start = Date.now();
await A.js("document.getElementById('listen').click()");
await A.waitFor("/Listening|failed|no Pkarr/.test(document.getElementById('listen-status').innerText)", 90000);
console.log(`[${ms()}] A: ${await text(A, "listen-status")} (${Date.now() - start} ms)`);
const listenedAt = Date.now();
const topic = (await text(A, "listen-status")).match(/_topic ([0-9a-f]{32})/)[1];
start = Date.now();
await B.js(`document.getElementById('target').value = ${JSON.stringify(A.key)}; document.getElementById('connect').click()`);
await B.waitFor(`/Connected|did not answer|No /.test(document.getElementById('connect-status').innerText)`, 120000);
console.log(`[${ms()}] B: ${await text(B, "connect-status")} (${Date.now() - start} ms after Connect)`);
await B.js("document.getElementById('text').value = 'hello from B'; document.getElementById('send').click()");
await A.waitFor("document.getElementById('messages').innerText.includes('hello from B')", 20000).then(() => console.log(`[${ms()}] A got B's message`), e => console.log(e.message));

const E = await open("E");
const forged = await E.js(`(async () => {
  const { ed25519 } = await import("@noble/curves/ed25519.js");
  const { defaultRelayUrls } = await import("trystero");
  const { sealKnock } = await import("./knock.js");
  const { createNostr } = await import("./nostr.js");
  const enc = new TextEncoder();
  const sha = async t => Array.from(new Uint8Array(await crypto.subtle.digest("SHA-256", enc.encode(t))), b => b.toString(16).padStart(2, "0")).join("");
  const topic = ${JSON.stringify(topic)};
  const ranked = await Promise.all(defaultRelayUrls.map(async url => ({ url, rank: await sha(topic + url) })));
  const relays = ranked.sort((a, b) => (a.rank < b.rank ? -1 : 1)).slice(0, 6).map(x => x.url);
  const tag = await sha("pkarr-p2p-knock/v1:" + topic);
  const kind = 20000 + (parseInt(tag.slice(0, 8), 16) % 10000);
  const nostr = createNostr(relays);
  await nostr.connected(2, 8000);
  await nostr.publish(kind, [["x", tag]], btoa("x".repeat(120)));
  const knock = { topic, room: "ab".repeat(16), from: ${JSON.stringify(B.key)}, at: Date.now(), signature: "00".repeat(64) };
  await nostr.publish(kind, [["x", tag]], await sealKnock(${JSON.stringify(A.key)}, knock));
  await new Promise(r => setTimeout(r, 3000));
  return "sent";
})()`);
await new Promise(r => setTimeout(r, 4000));
console.log(`[${ms()}] impostor E ${forged} 2 bad knocks; A knock log:\n  ` + (await logOf(A)).split("\n").filter(l => /  knock  /.test(l)).map(l => l.slice(0, 200)).join("\n  "));

const C = await open("C");
const wait = 70000 - (Date.now() - listenedAt); if (wait > 0) await new Promise(r => setTimeout(r, wait));
start = Date.now();
await C.js(`document.getElementById('target').value = ${JSON.stringify(A.key)}; document.getElementById('connect').click()`);
await C.waitFor(`/Connected|did not answer|No /.test(document.getElementById('connect-status').innerText)`, 120000);
console.log(`[${ms()}] C (${Math.round((start - listenedAt) / 1000)} s after A listened): ${await text(C, "connect-status")} (${Date.now() - start} ms after Connect)`);
console.log(`B nostr log: ` + (await logOf(B)).split("\n").filter(l => /after it connected|published event|accepted|REJECTED/.test(l)).map(l => l.slice(14, 150)).join(" || "));
for (const p of [A, B, C, E]) { writeFileSync(`${outDir}/log2-${p.name}.txt`, await logOf(p)); console.log(`${p.name} errors: ${[...new Set(p.errors)].slice(0, 3).join(" | ") || "(none)"}`); }
await A.shot("K2-A-desktop"); await B.shot("K2-B-mobile", 390);
for (const p of [A, B, C, E]) p.close(); process.exit(0);
