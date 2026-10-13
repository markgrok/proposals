// A static server for this directory on localhost, cross-origin isolated (for WasmFS's pthreads).
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
const dir = path.dirname(new URL(import.meta.url).pathname);
const types = { '.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript', '.wasm': 'application/wasm' };
export function serve(port) {
    const server = http.createServer((req, res) => {
        const p = path.join(dir, decodeURIComponent(new URL(req.url, 'http://x').pathname));
        if (!p.startsWith(dir) || !fs.existsSync(p) || fs.statSync(p).isDirectory()) { res.writeHead(404); res.end(); return; }
        res.writeHead(200, {
            'Content-Type': types[path.extname(p)] || 'application/octet-stream',
            'Cache-Control': 'no-store',
            'Cross-Origin-Opener-Policy': 'same-origin',
            'Cross-Origin-Embedder-Policy': 'require-corp',
        });
        fs.createReadStream(p).pipe(res);
    });
    return new Promise((r) => server.listen(port, '127.0.0.1', () => r(server)));
}
if (process.argv[1] === new URL(import.meta.url).pathname) serve(Number(process.argv[2] || 8642)).then(() => console.log('serving'));
