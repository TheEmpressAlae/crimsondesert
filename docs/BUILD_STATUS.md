# Build status

- Configuration: `Release|x64`
- Toolchain: Visual Studio Build Tools 2022 17.14, MSVC 19.44 (`v143`)
- Runtime linkage: static
- PE dependencies: `KERNEL32.dll`, `USER32.dll`
- Public output: `MarkerTeleport.asi`
- Runtime-tested packaged ASI SHA-256: `6B99E7F2F5627906B18CC0851C1B0FCFB6BD31EADAFC3C61E087A76609D4B12E`
- DMM ZIP SHA-256: `0AF1D71765A362D5EFF8CDB05FD10DFE252AF28B8FEDCECF689160B714037E37`
- Post-test verification rebuild SHA-256: `B11322C0C122B2360D8D5623C6642B92BEBB814F9FCBD414F39F0CE1BB8C04DF` (not runtime-tested; PE output is not reproducible byte-for-byte)

The current-build bridge compiles with zero errors and zero warnings. It validates expected AOB match counts before installing guarded inline hooks and remains fail-closed when those checks drift.

## Runtime verification

- Game: Crimson Desert BuildID `24568997`, executable version `1.0.0.2289`
- DMM: 1.5.8 GUI ingestion using adjacent `MarkerTeleport.asi` and `MarkerTeleport.ini`
- Loader: Ultimate ASI Loader 9.7.2
- Attribution: exact packaged ASI hash loaded; the reference teleport ASI and old `MarkerTeleportASI` module were absent
- Log: `READY` followed by two `Teleport result: success` entries in one game process
- Visual results: open-marker teleport passed; targeted-marker teleport passed with normal terrain correction

Absent/stale-marker behavior and normal process teardown remain pending before the draft PR is promoted to release-ready.
