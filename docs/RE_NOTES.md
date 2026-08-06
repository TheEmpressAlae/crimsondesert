# Reverse-engineering notes

Source examined: OpenStorageAnywhere v1.6 ASI, SHA-256 `7D259E12D5666E3524EAB56F134C08417FCD242A724490A121C62F4D864D9ED5`.

These notes record behavior and interfaces for a clean-room reimplementation. RVAs refer to image base `0x180000000`.

## Recovered flow

- Initialization: `0x13C0`
- Configuration: `0x1470`
- Runtime/signature setup: `0x5F70`
- Teleport operation: `0x6880`
- Hook/trampoline installer: `0x6BF0`
- Guarded copy: `0x70F0`
- Protection expiry: `0x7230`
- Input loop: `0x72F3`; thread entry around `0x7870`
- Controller setup: `0x7C40`

The input loop polls about every 25 ms, debounces the configured teleport key, invokes teleport, and services the protection-expiry deadline.

Teleportation rejects missing, non-finite, implausible, and all-zero marker vectors. The observed transformation uses horizontal deltas from the current player position. Marker height is used when nonzero; otherwise the configured fallback height is substituted, followed by a small unresolved lift. The resulting 12-byte vector is guarded-written to two player-entity fields at offsets `0x90` and `0x1A0`.

Successful teleports can set a separately resolved protection byte to `1`. The expiry deadline is `GetTickCount64() + seconds * 1000`; the expiry path restores the byte to `0`. A duration of zero must avoid all protection-state access.

## Hook evidence

- Named mapping: `Local\\CrimsonDesert_PlayerEntity_v1`
- One exact seven-byte target sequence: `48 8B 46 08 48 89 F1`
- Observed hook lengths include 7 and 8 bytes.
- Detour/trampoline regions are arranged within the executable mapping, including offsets `0x100` and `0x200`.

## Current-build recovery

- Player capture AOB: `48 8B 06 C5 F8 11 88 B0 01 00 00`; expected match count: 1
- Marker capture AOB: `C5 FB 10 07 C5 FB 11 02 8B 47 08 89 42 08`; expected match count: 5
- World-origin prefix: `C5 F8 5C 05 rel32`; expected match count: 9, all resolving to one vector
- Protection AOB: `48 8B 46 08 48 89 F1`; expected match count: 1 when the no-height safety path is available
- Destination transform: `{marker.x - origin.x, selectedHeight + 10, marker.z - origin.z}`
- Player writes: 12-byte vector at offsets `0x90` and `0x1A0`

An earlier transcription used `0x190` for the first destination field. That write did not fault but affected unused state, causing logged false successes with no visible movement. Rechecking the reference instruction path established the correct first offset as `0x90`.

## Remaining runtime matrix

- Absent and stale marker behavior
- Repeated hotkey calls beyond the initial successful pair
- Mounted, loading, cutscene, interior, and fast-travel contexts
- Normal shutdown and subsequent clean relaunch

The implementation remains fail-closed when required current-build resolver counts or origin-agreement checks fail.
