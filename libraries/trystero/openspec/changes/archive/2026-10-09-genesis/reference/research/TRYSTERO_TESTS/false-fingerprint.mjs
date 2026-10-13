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
const signal = new URL(url).searchParams.get("signal");
const A = await open("A"), F = await open("F");
await A.js("document.getElementById('listen').click()");
await A.waitFor("document.getElementById('listen-status').className.includes('ok')");
const topic = (await text(A, "listen-status")).match(/_topic ([0-9a-f]{32})/)[1];
console.log(`[${ms()}] A listening on ${topic}`);
await F.js(`(async () => {
  const { ed25519 } = await import("@noble/curves/ed25519.js");
  const joinRoom = (await import("./ws-relay-strategy.js")).wsRelayStrategy({ steadyAnnounceIntervalMs: 5000 });
  const secret = ed25519.utils.randomSecretKey();
  const { z32encode } = await import("./pkarr.js");
  const key = z32encode(ed25519.getPublicKey(secret));
  const fake = Array.from({ length: 32 }, () => "ab").join(":");
  const hex = b => Array.from(b, x => x.toString(16).padStart(2, "0")).join("");
  window.__liar = joinRoom({ appId: "pkarr-p2p-demo/v1", passive: true, relayConfig: { urls: [${JSON.stringify(signal)}] } }, ${JSON.stringify(topic)}, {
    onPeerHandshake: async (id, send, receive) => {
      await send({ key, nonce: "cd".repeat(32), fingerprint: fake });
      const { data: hello } = await receive();
      const message = new TextEncoder().encode("pkarr-p2p-auth/v1\\n" + ${JSON.stringify(topic)} + "\\n" + hello.nonce + "\\n" + key + "\\n" + fake);
      await send({ signature: hex(ed25519.sign(message, secret)) });
      await receive();
    },
  });
  window.__liarKey = key;
  return key;
})()`);
console.log(`[${ms()}] F joined with a valid key and signature but a false DTLS fingerprint`);
await A.waitFor("document.getElementById('log').innerText.includes('someone is in the middle')", 30000);
await new Promise(r => setTimeout(r, 15000));
console.log(`[${ms()}] A log:\n  ` + (await text(A, "log")).split("\n").slice(0, 5).join("\n  "));
console.log(`A peers: ${(await text(A, "peers")).replace(/\n/g, " | ")}`);
console.log(`A page errors: ${A.errors.join(" | ") || "(none)"}`);
A.close(); F.close(); process.exit(0);
