import * as assert from '../assert.js';

assert.throws(() => {
    new WebAssembly.Global({
        value: 'cocoa'
    });
}, TypeError, `WebAssembly.Global expects its 'value' field to be the string 'i32', 'i64', 'f32', 'f64', 'anyfunc', 'funcref', or 'externref'`);

assert.throws(() => {
    new WebAssembly.Global({
        value: 'anyfunc'
    }, {});
}, TypeError, `Argument value did not match the reference type`);
