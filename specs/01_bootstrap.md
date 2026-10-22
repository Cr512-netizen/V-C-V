# Milestone 1 — Bootstrap and runtime

C++20 skeleton: CMake + Ninja, ccache when installed, strict warnings, ASan/UBSan in Debug, FetchContent dependencies (GLFW, GLM, enkiTS, sol2, Catch2, toml++, nlohmann/json), a window/event loop, fixed-step timing, logging/config/jobs, an OpenGL clear-color stub renderer and Lua boot script. Vulkan instance/device/swapchain remains future work.

## Milestone 2.1 — Materials and block registry

Materials and block definitions load from `data/materials.json` and `data/blocks.json`. IDs and names are validated for uniqueness, material references are checked, colors and flags are validated, and each block expands to an 8×8×8 array of material IDs (512 voxels). Registry loading is transactional: invalid input throws without replacing a previously loaded registry.

## Build

Requirements: CMake 3.24+, Ninja, C++20 compiler, Git, Lua 5.4 development package and GLFW platform build dependencies.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/nuc_app
```


## Milestone 2.2 — CPU SVDAG

Added a CPU-side sparse voxel directed acyclic graph builder for cubic power-of-two grids. It interns identical leaf and branch nodes, supports voxel queries and decoding, and rejects invalid dimensions/buffers. The current implementation favors correctness and deduplication; large-grid build performance still needs profiling before production use.


## Milestone 2.3 — Deterministic terrain chunks

Added 32³ chunk storage and seeded height-field terrain generation using FastNoiseLite (OpenSimplex2). Terrain material IDs follow the shipped registry: air 0, stone 1, soil 2, grass 3, water 4. Generation is deterministic for a seed and chunk coordinate; settings validate before generation. Lua generation hooks and job-system integration remain follow-up work.
