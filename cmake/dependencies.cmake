# Third-party dependencies.
#
# Every dependency is pinned to an exact tag so that a fresh clone builds the
# same tree twice. Anything that ships its own CMakeLists.txt goes through
# FetchContent_MakeAvailable; the rest only needs its sources unpacked.

include(FetchContent)

find_program(NUC_CCACHE ccache)
if(NUC_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${NUC_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${NUC_CCACHE}")
    message(STATUS "ccache: ${NUC_CCACHE}")
else()
    message(STATUS "ccache: not found, incremental builds rely on the compiler alone")
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Some upstream projects ship no CMakeLists.txt at all, so they can only be
# populated and never advertised through FetchContent_MakeAvailable. CMP0169
# keeps the single-argument form working on CMake 4, where it is an error.
#
# The policy is set for this directory rather than pushed and popped around the
# call: FetchContent returns early when a download fails, and a push left without
# its pop makes CMake report a bogus policy imbalance on top of the real error.
#
# FetchContent_Populate writes <name>_SOURCE_DIR into the calling scope, so it is
# called at file scope here; wrapping it in a function silently empties the path.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_Declare(enkits GIT_REPOSITORY https://github.com/dougbinks/enkiTS.git GIT_TAG v1.11 GIT_SHALLOW TRUE)
FetchContent_Declare(sol2 GIT_REPOSITORY https://github.com/ThePhD/sol2.git GIT_TAG v3.3.1 GIT_SHALLOW TRUE)
FetchContent_Declare(catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.7.1 GIT_SHALLOW TRUE)
FetchContent_Declare(tomlplusplus GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git GIT_TAG v3.4.0 GIT_SHALLOW TRUE)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3 GIT_SHALLOW TRUE)
FetchContent_Declare(fastnoiselite GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git GIT_TAG v1.1.1 GIT_SHALLOW TRUE)

FetchContent_GetProperties(fastnoiselite)
if(NOT fastnoiselite_POPULATED)
    FetchContent_Populate(fastnoiselite)
endif()

FetchContent_MakeAvailable(glfw glm enkits sol2 catch2 tomlplusplus nlohmann_json)
