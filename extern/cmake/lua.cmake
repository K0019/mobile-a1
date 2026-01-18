# Lua CMake wrapper
# Builds Lua as a static library from the official source

set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/lua)

set(LUA_SOURCES
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/linit.c
    ${LUA_DIR}/liolib.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/loadlib.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/loslib.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lutf8lib.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
)

add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC ${LUA_DIR})

# Platform-specific definitions
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_compile_definitions(lua_static PRIVATE LUA_USE_LINUX)
elseif(APPLE)
    target_compile_definitions(lua_static PRIVATE LUA_USE_MACOSX)
endif()

# Silence warnings on MSVC
if(MSVC)
    target_compile_options(lua_static PRIVATE /wd4244 /wd4702 /wd4267)
endif()

# Create alias for consistent naming
add_library(Lua::Lua ALIAS lua_static)

# Export variables for consumers
set(LUA_INCLUDE_DIR ${LUA_DIR} CACHE PATH "Lua include directory")
set(LUA_LIBRARIES lua_static CACHE STRING "Lua library target")
