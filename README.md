# Marker Teleport ASI v1.1.0

Clean-room, teleport-only ASI project for Crimson Desert. The intended behavior is deliberately narrow: when the configured hotkey is pressed and a valid destination marker exists, move the active player to that marker. No storage functionality is included.

## Current status

Version 1.1.0 is a playable current-build release candidate. The isolated bridge resolves and guards the current player, destination marker, world origin, and temporary-protection interfaces. It refuses to arm when the expected signature counts or agreement checks fail.

The v1.0.0 bridge passed DMM 1.5.8 GUI ingestion and uncontaminated runtime attribution on Crimson Desert BuildID `24568997` (`1.0.0.2289`) with Ultimate ASI Loader 9.7.2. The exact v1.1.0 production ASI and F10 configuration were subsequently verified through DMM and in game: teleporting to a map object landed correctly, while teleporting to an open-map marker exposed an unresolved excessive-altitude defect. See `docs/BUILD_STATUS.md` for exact hashes and evidence boundaries.

Known issue: an open-map marker can resolve to an unsafe high-altitude destination even though the bridge reports `Teleport result: success`. Map-object destinations have landed correctly in the confirmed v1.1.0 test.

## Build

Open `MarkerTeleportASI.slnx` with Visual Studio and build `Release|x64`, or invoke MSBuild against `MarkerTeleportASI\MarkerTeleportASI.vcxproj`. Rename the resulting DLL to `MarkerTeleport.asi` for Ultimate ASI Loader.

The current bridge builds with Visual Studio Build Tools 2022 (`v143`) as `Release|x64`, uses the static C++ runtime, and has only Windows system DLL dependencies. See `docs/BUILD_STATUS.md`.

## Runtime files

- `MarkerTeleport.asi`
- `MarkerTeleport.ini`
- optional diagnostic `MarkerTeleport.log`

For DMM 1.5.8, import the adjacent same-stem ASI/INI pair through the DMM GUI. The release ZIP keeps both files at its root and does not bundle an ASI loader. The shipped teleport hotkey is `F10`; the shipped reload key is `F11`.

## Hotkey configuration

Edit `MarkerTeleport.ini` and set `Teleport.Hotkey` to a standard keyboard, mouse, navigation, modifier, numpad, browser, media, or application-launch key. Key names are case-insensitive. `A`–`Z`, top-row `0`–`9`, `F1`–`F24`, and `NUMPAD0`–`NUMPAD9` work directly; `NUM0`–`NUM9` and `KP0`–`KP9` are accepted aliases. For example, to switch the teleport key to numpad 9:

```ini
[Teleport]
Hotkey=NUMPAD9
```

Press the configured reload key (F11 by default) to apply an INI change while the game is running. Num Lock should be enabled when using a numpad digit.

See [docs/KEY_BINDINGS.md](docs/KEY_BINDINGS.md) for the complete name list. Advanced users may bind any Windows virtual-key value from `0x01` through `0xFE` with `VK:0xNN` (the alias `VK_0xNN` is also accepted).

## Clean-room boundary

The project reproduces user-visible behavior from public documentation and independently observed program behavior. It does not contain copied code, binary fragments, prose, or assets from OpenStorageAnywhere.

## License

Copyright TheEmpressAlae. Distributed under GPL-3.0; see `LICENSE`.
