# Lua mods

Each subdirectory in `mods/` may contain a `mod.json` manifest and an `init.lua` entry point.

Manifest fields: `id` (unique identifier), `version`, `api_version` (currently `1`), and optional `dependencies` (mod IDs that must load first).

Each mod receives its own Lua state. The sandbox omits filesystem, OS, package, debug, and file-loading libraries and enforces an instruction budget plus a wall-clock deadline for entry scripts. API tables are `log`, `config`, `registry`, `events`, `world`, and `ecs`.

The voxel API is currently an in-memory prototype store, not connected to persisted chunk data. Config values are strings stored per mod. Lifecycle dispatch and hot reload are planned for step 4.2.
