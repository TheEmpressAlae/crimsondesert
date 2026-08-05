# Marker Teleport ASI v1.0.0

Clean-room, teleport-only ASI project for Crimson Desert. The intended behavior is deliberately narrow: when the configured hotkey is pressed and a valid destination marker exists, move the active player to that marker. No storage functionality is included.

## Current status

Version 1.0.0 is a fail-closed implementation milestone, not a playable release. Configuration, lifecycle, input polling, debounce, logging, and the game-bridge boundary are implemented. The bridge intentionally refuses to arm until the current game build's player, marker, and protection signatures are independently verified.

Target under investigation: Crimson Desert 1.16.0 (August 2026).

## Build

Open `MarkerTeleportASI.slnx` with Visual Studio and build `Release|x64`, or invoke MSBuild against `MarkerTeleportASI\MarkerTeleportASI.vcxproj`. Rename the resulting DLL to `MarkerTeleportASI.asi` for Ultimate ASI Loader.

The fail-closed scaffold has been compiled successfully with Visual Studio Build Tools 2022 (`v143`) as `Release|x64`. See `docs/BUILD_STATUS.md`. The resulting developer ASI is not yet a functional teleport release because its current-build game bridge remains intentionally unresolved.

## Runtime files

- `MarkerTeleportASI.asi`
- `MarkerTeleportASI.ini`
- optional diagnostic `MarkerTeleportASI.log`

Place them beside `CrimsonDesert.exe` in `bin64` after runtime validation is complete.

## Clean-room boundary

The project reproduces user-visible behavior from public documentation and independently observed program behavior. It does not contain copied code, binary fragments, prose, or assets from OpenStorageAnywhere.

## License

Copyright TheEmpressAlae. Distributed under GPL-3.0; see `LICENSE`.
