addRunDependency('wasm-instantiate'); // we can't run yet
if (Module['module']) {
  var beforeInstantiate = performance.now();
  console.log('>>> beforeInstantiate: ', beforeInstantiate);
  WebAssembly.instantiate(Module['module'], info).then(function(instance) {
    var afterInstantiate = performance.now();
    console.log('>>> Module instantiation: ', afterInstantiate - beforeInstantiate, 'ms');
    console.timeStamp('After instantiation');
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    receiveInstance(instance);
    removeRunDependency('wasm-instantiate');
  }).catch(function(reason) {
    Module['printErr']('failed to asynchronously prepare wasm:\n  ' + reason);
  });
} else {
  WebAssembly.instantiate(getBinary(), info).then(function(output) {
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    receiveInstance(output.instance);
    removeRunDependency('wasm-instantiate');
  }).catch(function(reason) {
    Module['printErr']('failed to asynchronously prepare wasm:\n  ' + reason);
  });
}
return {}; // no exports yet; we'll fill them in later
