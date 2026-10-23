# Nuc-lVuc-l

C++20 runtime/engine skeleton, now well into the voxel-world implementation. Materials/blocks registry validation and a CPU SVDAG prototype, 32³ chunk storage, and deterministic FastNoiseLite terrain generation are implemented. The world renders through an OpenGL 3.3 core path with GPU chunk meshes, a free-fly camera and DDA dig/place editing; world saves are versioned and deterministic; and a rigid-body foundation with block fracture and debris exists. Vulkan raymarch rendering remains a later milestone. This is a test build, not a finished game. Build/test status is tracked by GitHub Actions.

## Build

Every dependency except the C++ toolchain is fetched or vendored by CMake, so a
fresh clone builds with three commands — no vcpkg, no manual `-D` flags:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

| Preset | Platform | Notes |
|---|---|---|
| `debug`, `release` | any Ninja host | plain Ninja, sanitizers where the toolchain supports them |
| `windows-debug`, `windows-release` | Windows | same, for MSVC |

On Windows a plain shell has no `cmake`, `ninja`, `cl` or `git`, so use the wrapper, which finds Visual Studio, enters `vcvars64` and then runs the same presets:

```powershell
tools/build-windows.ps1 -Preset windows-release
```

The wrapper switches the console to UTF-8 (`chcp 65001`) before configuring, and fails the build if Ninja reports zero header dependencies. This is not cosmetic: a non-English MSVC prints a localized `/showIncludes` line, and CMake stores that prefix in `CMakeFiles/rules.ninja` using the console code page. Under the default OEM code page (866 on a Russian Windows) the stored prefix is unreadable, Ninja stops matching it, and editing a header no longer rebuilds anything — `ninja: no work to do` with `#deps 0`. If you configure by hand, run `chcp 65001` first and check `ninja -t deps <object>` reports a non-zero count.

Lua 5.4.6 is vendored by `cmake/lua.cmake` (pinned URL and SHA-256). Distributions that forbid a bundled Lua can pass `-DUSE_SYSTEM_LUA=ON` to use `find_package(Lua 5.4)` instead; both paths expose the same `Lua::Lua` target. `cmake/dependencies.cmake` pins GLFW, GLM, enkiTS, sol2, Catch2, toml++, nlohmann/json and FastNoiseLite to exact tags.

Prerequisites: CMake 3.25+, Ninja, a C++20 compiler (MSVC 19.4x or GCC/Clang), and Git — FetchContent clones the dependencies. ccache is used automatically when it is on `PATH`.
