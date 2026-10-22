# Phase A — See the World

Status: reviewed and approved for implementation (2026-10-18).

## Goal
Show the generated voxel world in the application window, with a controllable free-fly camera and FPS logging. Keep the existing engine/core architecture and use the existing GLFW, OpenGL, GLM, chunk mesher, Catch2 and config parser. Do not begin Phase B work.

## Scope and interfaces

### `render/gl/mesh_upload.h/.cpp`
- Own one chunk's GPU resources (VAO, VBO, EBO); release them safely and disallow copying.
- Public API: `bool upload(const render::ChunkMesh&)`, `void draw() const`, `void reset() noexcept`, `bool ready() const noexcept`, `std::size_t index_count() const noexcept`.
- Vertex layout: position vec3, normal vec3, material-derived color vec3. Convert CPU material IDs to vertex colors during upload; do not require a texture dependency.
- Upload/draw/reset require a current OpenGL context. Empty meshes are valid and draw zero indices. Check GL errors and return failure rather than marking invalid buffers ready.

### `render/gl/gl_renderer.h/.cpp`
- Implement the existing `IRenderer` interface; no changes to the core interface.
- Accept a non-owning list of chunk draw records and a camera view-projection matrix through `set_view_projection(const glm::mat4&) noexcept`.
- Compile/link a GLSL 330 core shader using position, normal and color; check compile/link logs and fail `init()` cleanly on failure.
- Draw visible chunks using GPU mesh resources and per-chunk model transforms; enable depth testing. Never use immediate mode or fixed-function matrix calls.
- Add debug diagnostics for shader/link and OpenGL errors. Where a debug context is available, enable GL debug output; do not claim validation cleanliness without logs.

### `platform/camera.h/.cpp`
- Own position, yaw, pitch, FOV, aspect and near/far planes.
- Public API: `explicit Camera(glm::vec3 position = {16, 28, 64}, float aspect = 16.0F/9.0F)`, `void update(GLFWwindow*, double dt)`, `void set_aspect(float) noexcept`, `void set_rotation(float yaw, float pitch) noexcept`, `void move_local(const glm::vec3& direction, double dt) noexcept`, `glm::vec3 forward() const noexcept`, `glm::mat4 view() const`, `glm::mat4 projection() const`, `glm::mat4 view_projection() const`, `glm::vec3 position() const noexcept`.
- WASD moves horizontally relative to yaw; Space/Ctrl move vertically; mouse controls yaw/pitch with pitch clamped to avoid singularity. Capture/hide cursor while flying; Escape remains the application's existing exit key.
- Ignore non-positive dt and clamp movement dt to avoid teleporting after stalls. Camera math must be unit-testable without a display.

### Config and app wiring
- Add `renderer = "gl_mesh"` as the default visible renderer setting in `[render]`; keep `null` as a supported test/headless renderer. `vulkan` may be recognized as reserved but must report “not implemented” rather than silently pretending to render. Keep old `opengl_stub` as a compatibility alias only if needed.
- Main builds CPU chunk meshes, uploads them once after context creation, passes the camera matrix each frame, and releases GL resources before window/context destruction.
- Log FPS at one-second intervals. Keep simulation tick rate at 15 Hz; camera/render updates use frame delta and do not change simulation timing.
- Avoid introducing new third-party dependencies. Keep new source/header files under 200 lines each and each module single-purpose.

## Tests and acceptance criteria
1. Camera unit tests: initial view-projection is finite; aspect ratio changes projection; yaw changes forward direction; pitch remains clamped; zero/negative dt does not move camera.
2. Mesh upload tests: empty mesh is safe; index count matches CPU mesh; reset is idempotent. GL-dependent tests may be a separate context-backed smoke test and must skip with an explicit reason if CI has no display.
3. Renderer tests: shader source/interface contract is checked; context-backed smoke test creates a hidden GLFW window, renders a deterministic test mesh, reads a pixel and proves the frame is not only the clear color. No display-dependent test may be reported as passed if skipped.
4. Run Ninja build and `ctest --output-on-failure` in Debug with ASan/UBSan where supported and Release CI. Preserve existing tests.
5. Windows and Linux CI green; inspect compile/link/debug logs on failures before guessing.
6. Update `ROADMAP.md` with evidence and exact status, then commit the complete phase. Do not start Phase B until all above acceptance criteria pass.

## Review checklist
- [x] Interface names/signatures fit current namespace and build conventions.
- [x] OpenGL 3.3 entry points must be resolved with GLFW proc addresses in a small GL-only loader; do not assume modern GL symbols are directly linked on Windows.
- [x] GL resources are destroyed before the GLFW context.
- [x] Camera tests are display-independent; pixel smoke test is explicit about headless limitations.
- [x] No Vulkan implementation, editing, saves, physics, or networking work is mixed into Phase A.

## Review notes
Initial repository review found an existing CPU chunk mesher and an incomplete compatibility-profile renderer using immediate mode. The current renderer must be replaced, not extended with more fixed-function code. The app currently has no camera abstraction and config defaults to `opengl_stub`. Existing CI builds Linux and Windows, but a true pixel smoke test may require Xvfb/software GL on Linux and a hidden window on Windows; CI capability must be verified before final acceptance.

### Review decision
The spec is consistent with the current codebase: it preserves `IRenderer`, uses the existing GLM/GLFW/Catch2 dependencies, and replaces the incomplete fixed-function path rather than expanding it. One implementation detail is mandatory: resolve OpenGL 3.3 functions via `glfwGetProcAddress` to support Windows drivers without an extra loader dependency. Camera math and local movement are testable without GLFW initialization; only input polling requires a live window. Proceed with Phase A only.
