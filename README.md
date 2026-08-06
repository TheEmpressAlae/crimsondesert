# Marker Teleport ASI v1.0.0

Clean-room, teleport-only ASI project for Crimson Desert. The intended behavior is deliberately narrow: when the configured hotkey is pressed and a valid destination marker exists, move the active player to that marker. No storage functionality is included.

## Current status

Version 1.0.0 is a playable current-build candidate. The isolated bridge resolves and guards the current player, destination marker, world origin, and temporary-protection interfaces. It refuses to arm when the expected signature counts or agreement checks fail.

The exact packaged test-5 binary passed DMM 1.5.8 GUI ingestion and uncontaminated runtime attribution on Crimson Desert BuildID `24568997` (`1.0.0.2289`) with Ultimate ASI Loader 9.7.2. Open-marker and targeted-marker teleports were visually confirmed. Absent/stale-marker behavior and clean shutdown remain final release-gate checks, so the follow-up PR remains a draft.

## Build

Open `MarkerTeleportASI.slnx` with Visual Studio and build `Release|x64`, or invoke MSBuild against `MarkerTeleportASI\MarkerTeleportASI.vcxproj`. Rename the resulting DLL to `MarkerTeleport.asi` for Ultimate ASI Loader.

The current bridge builds with Visual Studio Build Tools 2022 (`v143`) as `Release|x64`, uses the static C++ runtime, and has only Windows system DLL dependencies. See `docs/BUILD_STATUS.md`.

## Runtime files

- `MarkerTeleport.asi`
- `MarkerTeleport.ini`
- optional diagnostic `MarkerTeleport.log`

For DMM 1.5.8, import the adjacent same-stem ASI/INI pair through the DMM GUI. The release ZIP keeps both files at its root and does not bundle an ASI loader.

## Clean-room boundary

The project reproduces user-visible behavior from public documentation and independently observed program behavior. It does not contain copied code, binary fragments, prose, or assets from OpenStorageAnywhere.

## License

Copyright TheEmpressAlae. Distributed under GPL-3.0; see `LICENSE`.
