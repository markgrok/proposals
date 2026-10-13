// Probe: does a terminated worker keep writing through its FileSystemSyncAccessHandle after a new
// worker has acquired the file's lock? Role 'writer' increments a counter in the file forever (busy
// loop, never yielding to the event loop); role 'reader' acquires the handle and samples it.
onmessage = async (e) => {
    const root = await navigator.storage.getDirectory();
    const fh = await root.getFileHandle('zombie', { create: true });
    const t0 = performance.now();
    let ah;
    for (let i = 0; ; ++i) {
        try { ah = await fh.createSyncAccessHandle(); break; } catch (err) { if (i > 2000) throw err; await new Promise((r) => setTimeout(r, 5)); }
    }
    const acquired = performance.now() - t0;
    const b = new Float64Array(1);
    if (e.data === 'writer') {
        ah.truncate(0);
        postMessage({ started: true });
        for (let n = 1; ; ++n) { b[0] = n; ah.write(b, { at: 0 }); if (n % 1000 === 0) ah.flush(); }
    } else {
        const samples = [];
        for (let i = 0; i < 6; ++i) {
            ah.read(b, { at: 0 });
            samples.push([Math.round(performance.now() - t0), b[0]]);
            await new Promise((r) => setTimeout(r, 500));
        }
        postMessage({ acquired, samples });
    }
};
