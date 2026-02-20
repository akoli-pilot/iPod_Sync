const { contextBridge } = require('electron');
const path = require('path');

let native;
try {
  native = require(path.join(__dirname, '..', 'build', 'Release', 'ipod_native.node'));
} catch (err) {
  console.warn('Native addon not loaded. Run npm run build:native. Error:', err.message);
}

const listDevices = async () => {
  if (!native || typeof native.listDevices !== 'function') return [];
  // Native call is synchronous; wrap to keep renderer API promise-based.
  return native.listDevices();
};

const api = { listDevices };

if (contextBridge && typeof contextBridge.exposeInMainWorld === 'function') {
  contextBridge.exposeInMainWorld('ipod', api);
} else {
  // Fallback
  globalThis.ipod = api;
}
