# Build status

- Configuration: `Release|x64`
- Toolchain: Visual Studio Build Tools 2022 17.14, MSVC 19.44 (`v143`)
- Runtime linkage: static
- PE dependencies: `KERNEL32.dll`, `USER32.dll`
- Output SHA-256: `1258DC3C3E8C83127D001CB2DA82CE2244A314ABA3405E0F51F3C696B8E0D4E8`

The compiled artifact is a developer scaffold. Its game bridge remains fail-closed because the current-build marker, player, safety-context, and protection resolvers are not yet implemented. Loading it should create a diagnostic log and report that teleport is disabled; it must not be represented as a functional release.
