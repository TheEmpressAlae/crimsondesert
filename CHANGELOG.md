# Changelog

## 1.0.0 — 2026-08-05

- Established the clean-room MarkerTeleport project.
- Added an isolated x64 ASI lifecycle, configuration loader, input polling, debounce, and diagnostic logging.
- Recovered the current-build player, marker, origin, and temporary-protection interfaces with guarded match-count validation.
- Added thread-safe inline hooks and a fail-closed game bridge that prevents player-memory writes until every required resolver is validated.
- Corrected the primary player destination offset from the non-faulting but ineffective `0x190` transcription to the verified `0x90` field.
- Standardized the public DMM identity as `MarkerTeleport` across the ASI, INI, log, build output, and package.
- Made the diagnostic log readable during live testing and recorded per-hotkey bridge results.
- Verified DMM ingestion, exact-binary attribution, and visible open/targeted marker teleports on Crimson Desert BuildID `24568997`.
- Added a Visual Studio 2022 `v143` project with static C++ runtime linkage.
