// The two waits the C++ side awaits as promises; everything else is called from C++ through emscripten::val.
Module.waitIceComplete = peer => new Promise(resolve => {
    if (peer.iceGatheringState === 'complete') return resolve();
    peer.addEventListener('icegatheringstatechange', () => peer.iceGatheringState === 'complete' && resolve());
});
Module.waitOpen = socket => new Promise((resolve, reject) => {
    socket.addEventListener('open', () => resolve(), {once: true});
    socket.addEventListener('error', () => reject(new Error('signaling socket failed')), {once: true});
});
Module.logFailure = (promise, what) => promise.catch(error => {
    document.getElementById('log').append('browser: ' + what + ' failed: ' + error.message + '\n');
});
