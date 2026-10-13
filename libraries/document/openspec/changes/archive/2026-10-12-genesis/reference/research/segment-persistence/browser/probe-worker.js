// Which storage APIs a dedicated worker has, in this engine.
onmessage = async () => {
    const out = {};
    const root = await navigator.storage.getDirectory();
    const fh = await root.getFileHandle('probe', { create: true });
    out.createSyncAccessHandle = typeof fh.createSyncAccessHandle === 'function';
    out.createWritable = typeof fh.createWritable === 'function';
    try {
        const ah = await fh.createSyncAccessHandle();
        out.flushReturns = String(ah.flush());
        out.getSizeType = typeof ah.getSize();
        ah.close();
    } catch (e) { out.syncError = String(e); }
    try {
        const a = await fh.createSyncAccessHandle({ mode: 'readwrite-unsafe' });
        const b = await fh.createSyncAccessHandle({ mode: 'readwrite-unsafe' });
        out.readwriteUnsafeTwoHandles = true;
        a.close(); b.close();
    } catch (e) { out.readwriteUnsafeTwoHandles = String(e.name); }
    const db = await new Promise((res, rej) => { const r = indexedDB.open('probe', 1); r.onupgradeneeded = () => r.result.createObjectStore('s'); r.onsuccess = () => res(r.result); r.onerror = () => rej(r.error); });
    for (const d of ['strict', 'relaxed', 'default', undefined]) {
        const tx = d ? db.transaction('s', 'readwrite', { durability: d }) : db.transaction('s', 'readwrite');
        out[`durability(${d ?? 'omitted'})`] = String(tx.durability);
        out.commitMethod = typeof tx.commit === 'function';
        tx.abort();
    }
    out.storageBuckets = typeof navigator.storageBuckets !== 'undefined';
    postMessage(out);
};
