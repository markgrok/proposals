// The few closures the C++ side needs: event handlers bound to C++ functions, promise settling and logging.
Module.bind = (name, ...args) => event => Module[name](...args, event);
Module.settle = promise => promise.then(value => ({ok: true, value}), error => ({ok: false, error: String(error?.message ?? error)}));
Module.guard = (promise, what) => promise.catch(error => Module.log('error', what + ' failed: ' + (error?.message ?? error)));
Module.log = (category, text) => {
    const line = `[${(performance.now() / 1000).toFixed(2)}s] ${category}: ${text}`;
    console.log(line);
    document.getElementById('log').append(line + '\n');
};
