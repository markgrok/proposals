// Prints the cost and crash tables from results/*.jsonl.
import fs from 'node:fs';
import path from 'node:path';
const dir = path.join(path.dirname(new URL(import.meta.url).pathname), 'results');
const read = (f) => fs.existsSync(path.join(dir, f)) ? fs.readFileSync(path.join(dir, f), 'utf8').trim().split('\n').filter(Boolean).map(JSON.parse) : [];
const f = (x) => x == null ? '-' : x < 10 ? x.toFixed(2) : x.toFixed(1);
console.log('cost: median / p99 ms (dirty scan or digest median ms), bytes per commit');
const cost = ['chromium', 'webkit', 'firefox'].flatMap((e) => read(`cost-${e}.jsonl`));
for (const r of cost) console.log(`${r.engine.padEnd(9)} ${r.strategy.padEnd(12)} ${String(r.mb).padStart(3)} MB  ${f(r.median).padStart(6)} / ${f(r.p99).padStart(6)}  (${f(r.scanMedian)})  ${Math.round(r.bytesMean)}${r.error ? ' ' + r.error : ''}`);
for (const r of read('wasmfs.jsonl')) console.log(`${r.engine.padEnd(9)} ${r.strategy.padEnd(12)} ${String(r.mb).padStart(3)} MB  ${f(r.median).padStart(6)} / ${f(r.p99).padStart(6)}  ${Math.round(r.bytesMean)}`);
console.log('crash: consistent / torn / lost / hang');
for (const r of read('crash.jsonl')) console.log(`${r.engine.padEnd(9)} ${r.kill.padEnd(9)} ${(r.s).padEnd(12)} n=${r.n} ${r.consistent} / ${r.torn} / ${r.lost} / ${r.hang}${r.commitsTotal ? ' commits ' + r.commitsTotal : ''}`);
