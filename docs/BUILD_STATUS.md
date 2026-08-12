# Build status

- Version: `1.1.0`
- Configuration: `Release|x64`
- Toolchain: Visual Studio Build Tools 2022 17.14, MSVC 19.44 (`v143`)
- Runtime linkage: static
- PE architecture: x64 DLL
- PE dependencies: `KERNEL32.dll`, `USER32.dll`
- Public output: `MarkerTeleport.asi`
- Production ASI SHA-256: `CA69B4D56795425E7E9DE2ECAE23A33308209D71D75FDEE831D83C4195FFBE9D`
- Production INI SHA-256: `1DF8C4CE69195837C9BD483BA96280688BE4A88B31E5D839EC5EE09BF47F971B`
- Production DMM ZIP SHA-256: `FEF4FBD5D53BFC7CF7D24328088619138A49FD0F293C131C14A0F05B27EB3DFB`

The v1.1.0 production build completed with zero errors and zero warnings. It embeds the `MarkerTeleport v1.1.0` startup identity, retains only Windows system DLL dependencies, and ships `Hotkey=F10` with `ReloadKey=F11`. The exact production ASI and INI hashes listed above were confirmed in the active game installation.

## v1.1.0 keybinding verification

- User-tested candidate ASI SHA-256: `C8A157733465DDD2DD70F6FE7DC28ADD3D9EBF07E9689896DE26D2D9D47B748B`
- User-tested DMM ZIP SHA-256: `A9FB684A2BD9604BDC99C82136921BC7BDF2D27278F4A8671758F391F69170F5`
- Test-package INI: `Hotkey=NUMPAD9`
- Dynamic result: DMM ingestion and in-game NUMPAD9 input/teleport passed by user confirmation
- Static parser coverage: alphanumeric, function, numpad, navigation, modifier, OEM punctuation, media, mouse, named `VK_` prefix, raw `VK:0xNN`/`VK_0xNN`, and invalid fallback

The exact production ASI was exercised in game with the shipped F10 binding. The log recorded `READY` and two `Teleport result: success` entries. A map-object destination landed correctly; an open-map marker sent the player to an excessive altitude. The latter is an unresolved destination-height defect, and it also demonstrates that the current success result confirms the teleport write rather than validating the final landing position. Earlier candidate testing separately established NUMPAD9 input. No claim is made that every documented key was individually tested in game.

## v1.0.0 runtime verification

- Runtime-tested packaged ASI SHA-256: `6B99E7F2F5627906B18CC0851C1B0FCFB6BD31EADAFC3C61E087A76609D4B12E`
- DMM ZIP SHA-256: `0AF1D71765A362D5EFF8CDB05FD10DFE252AF28B8FEDCECF689160B714037E37`
- Post-test verification rebuild SHA-256: `B11322C0C122B2360D8D5623C6642B92BEBB814F9FCBD414F39F0CE1BB8C04DF` (not runtime-tested; PE output is not reproducible byte-for-byte)
- Game: Crimson Desert BuildID `24568997`, executable version `1.0.0.2289`
- DMM: 1.5.8 GUI ingestion using adjacent `MarkerTeleport.asi` and `MarkerTeleport.ini`
- Loader: Ultimate ASI Loader 9.7.2
- Attribution: exact packaged ASI hash loaded; the reference teleport ASI and old `MarkerTeleportASI` module were absent
- Log: `READY` followed by two `Teleport result: success` entries in one game process
- Visual results: open-marker teleport passed; targeted-marker teleport passed with normal terrain correction

Absent/stale-marker behavior and normal process teardown have not been separately verified.
