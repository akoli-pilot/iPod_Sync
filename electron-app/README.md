# An Electron iPod Sync Tool

Minimal Electron shell that calls a Node-API native addon to enumerate iPods.

## Prereqs (Linux only)
- Node.js 18+
- CMake 3.18+
- Build tools (make, gcc/g++)
- Libraries: libimobiledevice, libplist, libudev (e.g., `sudo apt install libimobiledevice-dev libplist-dev libudev-dev`)

## Install & run
```
cd electron-app
npm install
npm run build:native
npm start
```

## Next steps
- Enrich disk-mode detection (parse `iPod_Control/Device/SysInfo`, fetch capacity via `statvfs`).
- Map device models using the enums in `foo_dop/ipod_manager.h`.
- Add sync/read APIs by adapting `foo_dop` logic into the addon.
