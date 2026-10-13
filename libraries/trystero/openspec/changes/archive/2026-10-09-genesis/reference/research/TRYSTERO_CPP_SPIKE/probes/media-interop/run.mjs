const target = await (await fetch("http://127.0.0.1:9336/json/new?about:blank", { method: "PUT" })).json();
const ws = new WebSocket(target.webSocketDebuggerUrl);
let id = 0; const pending = new Map(); const errors = [];
const send = (method, params = {}) => new Promise(r => { const i = ++id; pending.set(i, r); ws.send(JSON.stringify({ id: i, method, params })); });
ws.onmessage = m => { const d = JSON.parse(m.data); if (d.id && pending.has(d.id)) { pending.get(d.id)(d.result); pending.delete(d.id); } if (d.method === "Runtime.exceptionThrown") errors.push(d.params.exceptionDetails.exception?.description ?? d.params.exceptionDetails.text); };
await new Promise(r => (ws.onopen = r));
await send("Runtime.enable"); await send("Page.enable");
await send("Page.navigate", { url: "http://127.0.0.1:8771/index.html" });
const js = async e => (await send("Runtime.evaluate", { expression: e, returnByValue: true })).result.value;
const end = Date.now() + 30000;
while (Date.now() < end) { const t = await js("document.getElementById('log')?.innerText ?? ''"); if ((t.match(/native counts/g) ?? []).length >= 4) break; await new Promise(r => setTimeout(r, 500)); }
console.log(await js("document.getElementById('log').innerText"));
console.log("page errors:", errors.join(" | ") || "(none)");
ws.close(); process.exit(0);
