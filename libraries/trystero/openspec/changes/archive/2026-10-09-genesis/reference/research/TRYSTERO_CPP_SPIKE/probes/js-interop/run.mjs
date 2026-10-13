// Opens a C++ peer and a Trystero JS peer in one room and waits until each has received the other's actions.
const [, , selfId = '', label = ''] = process.argv;
const room = 'probe-' + Math.random().toString(36).slice(2, 10);
const base = 'http://127.0.0.1:8772';
async function open(path) {
    const target = await (await fetch('http://127.0.0.1:9337/json/new?about:blank', {method: 'PUT'})).json();
    const ws = new WebSocket(target.webSocketDebuggerUrl);
    let id = 0; const pending = new Map(); const errors = [];
    const send = (method, params = {}) => new Promise(r => { const i = ++id; pending.set(i, r); ws.send(JSON.stringify({id: i, method, params})); });
    ws.onmessage = m => { const d = JSON.parse(m.data);
        if (d.id && pending.has(d.id)) { pending.get(d.id)(d.result); pending.delete(d.id); }
        if (d.method === 'Runtime.exceptionThrown') errors.push('EXCEPTION ' + (d.params.exceptionDetails.exception?.description ?? d.params.exceptionDetails.text));
        if (d.method === 'Runtime.consoleAPICalled' && ['error', 'warning'].includes(d.params.type)) errors.push(`console.${d.params.type} ` + d.params.args.map(a => a.value ?? a.description).join(' ')); };
    await new Promise(r => (ws.onopen = r));
    await send('Runtime.enable');
    await send('Page.navigate', {url: base + path});
    const text = async () => (await send('Runtime.evaluate', {expression: "document.getElementById('log')?.innerText ?? ''", returnByValue: true})).result.value;
    return {text, errors, close: () => send('Page.close').catch(() => {})};
}
const t0 = Date.now();
const cpp = await open(`/cpp.html?room=${room}&self=${selfId}`);
const js = await open(`/js.html?room=${room}`);
const done = async () => {
    const [c, j] = [await cpp.text(), await js.text()];
    return /received chat .*hello from Trystero JS/.test(c) && /received drink .*negroni/.test(c) && /received chat .*echo: hello from C\+\+/.test(c)
        && /received chat from .*hello from C\+\+/.test(j) && /received drink from .*cafezinho/.test(j) && /received chat from .*echo: hello from Trystero JS/.test(j);
};
let ok = false;
while (Date.now() - t0 < 90000) { if (await done()) { ok = true; break; } await new Promise(r => setTimeout(r, 500)); }
await new Promise(r => setTimeout(r, 2000));
console.log(`===== ${label} room ${room}: ${ok ? 'PASS' : 'FAIL'} after ${((Date.now() - t0) / 1000).toFixed(1)}s`);
console.log('----- C++ peer'); console.log(await cpp.text()); if (cpp.errors.length) console.log(cpp.errors.join('\n'));
console.log('----- JS peer'); console.log(await js.text()); if (js.errors.length) console.log(js.errors.join('\n'));
await cpp.close(); await js.close();
process.exit(ok ? 0 : 1);
