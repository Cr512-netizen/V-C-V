# Lua 5.4, vendored by default.
#
# The upstream tarball carries a Makefile and no CMakeLists.txt, so the
# interpreter core is compiled here from an explicit source list. The standalone
# drivers (lua.c, luac.c) are deliberately excluded: this project embeds Lua, it
# never needs the REPL binary.
#
# USE_SYSTEM_LUA=ON switches to find_package(Lua 5.4) for distributions that
# forbid vendored copies. Both paths expose the same Lua::Lua target.

option(USE_SYSTEM_LUA "Use a system Lua 5.4 instead of the vendored copy" OFF)

set(NUC_LUA_VERSION "5.4.6")
set(NUC_LUA_URL "https://www.lua.org/ftp/lua-${NUC_LUA_VERSION}.tar.gz")
set(NUC_LUA_SHA256 "7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88")

if(USE_SYSTEM_LUA)
    find_package(Lua 5.4 REQUIRED)
    add_library(nuc_lua INTERFACE)
    target_include_directories(nuc_lua INTERFACE ${LUA_INCLUDE_DIR})
    target_link_libraries(nuc_lua INTERFACE ${LUA_LIBRARIES})
    message(STATUS "Lua: system ${LUA_VERSION_STRING}")
else()
    FetchContent_Declare(lua
        URL ${NUC_LUA_URL}
        URL_HASH SHA256=${NUC_LUA_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_GetProperties(lua)
    if(NOT lua_POPULATED)
        # Called at file scope on purpose: FetchContent_Populate publishes
        # lua_SOURCE_DIR into the calling scope, not the parent one.
        FetchContent_Populate(lua)
    endif()

    set(NUC_LUA_SOURCES
        ${lua_SOURCE_DIR}/src/lapi.c
        ${lua_SOURCE_DIR}/src/lauxlib.c
        ${lua_SOURCE_DIR}/src/lbaselib.c
        ${lua_SOURCE_DIR}/src/lcode.c
        ${lua_SOURCE_DIR}/src/lcorolib.c
        ${lua_SOURCE_DIR}/src/lctype.c
        ${lua_SOURCE_DIR}/src/ldblib.c
        ${lua_SOURCE_DIR}/src/ldebug.c
        ${lua_SOURCE_DIR}/src/ldo.c
        ${lua_SOURCE_DIR}/src/ldump.c
        ${lua_SOURCE_DIR}/src/lfunc.c
        ${lua_SOURCE_DIR}/src/lgc.c
        ${lua_SOURCE_DIR}/src/linit.c
        ${lua_SOURCE_DIR}/src/liolib.c
        ${lua_SOURCE_DIR}/src/llex.c
        ${lua_SOURCE_DIR}/src/lmathlib.c
        ${lua_SOURCE_DIR}/src/lmem.c
        ${lua_SOURCE_DIR}/src/loadlib.c
        ${lua_SOURCE_DIR}/src/lobject.c
        ${lua_SOURCE_DIR}/src/lopcodes.c
        ${lua_SOURCE_DIR}/src/loslib.c
        ${lua_SOURCE_DIR}/src/lparser.c
        ${lua_SOURCE_DIR}/src/lstate.c
        ${lua_SOURCE_DIR}/src/lstring.c
        ${lua_SOURCE_DIR}/src/lstrlib.c
        ${lua_SOURCE_DIR}/src/ltable.c
        ${lua_SOURCE_DIR}/src/ltablib.c
        ${lua_SOURCE_DIR}/src/ltm.c
        ${lua_SOURCE_DIR}/src/lundump.c
        ${lua_SOURCE_DIR}/src/lutf8lib.c
        ${lua_SOURCE_DIR}/src/lvm.c
        ${lua_SOURCE_DIR}/src/lzio.c)

    add_library(nuc_lua STATIC ${NUC_LUA_SOURCES})
    target_include_directories(nuc_lua PUBLIC ${lua_SOURCE_DIR}/src)
    if(MSVC)
        target_compile_definitions(nuc_lua PRIVATE LUA_USE_WINDOWS _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_definitions(nuc_lua PRIVATE LUA_USE_POSIX)
    endif()
    set_target_properties(nuc_lua PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON)
    # Third-party C code: never fail our build on its warnings.
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(nuc_lua PRIVATE -w)
    endif()
    message(STATUS "Lua: vendored ${NUC_LUA_VERSION}")
endif()

add_library(Lua::Lua ALIAS nuc_lua)
