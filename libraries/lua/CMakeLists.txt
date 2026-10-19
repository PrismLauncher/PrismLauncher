cmake_minimum_required(VERSION 3.15)
project(lua C)

if(Launcher_USE_SYSTEM_LUA)
    add_library(Launcher_lua INTERFACE)
    target_include_directories(Launcher_lua INTERFACE ${LUA_INCLUDE_DIR})
    target_link_libraries(Launcher_lua INTERFACE ${LUA_LIBRARIES})
    return()
endif()

set(LUA_SOURCES
    src/lapi.c
    src/lcode.c
    src/lctype.c
    src/ldebug.c
    src/ldo.c
    src/ldump.c
    src/lfunc.c
    src/lgc.c
    src/llex.c
    src/lmem.c
    src/lobject.c
    src/lopcodes.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/ltable.c
    src/ltm.c
    src/lundump.c
    src/lvm.c
    src/lzio.c

    src/lauxlib.c
    src/lbaselib.c
    src/lcorolib.c
    src/ldblib.c
    src/liolib.c
    src/lmathlib.c
    src/loadlib.c
    src/loslib.c
    src/lstrlib.c
    src/ltablib.c
    src/lutf8lib.c
    src/linit.c
)

add_library(Launcher_lua STATIC ${LUA_SOURCES})
target_include_directories(Launcher_lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(UNIX)
    target_compile_definitions(Launcher_lua PRIVATE LUA_USE_POSIX)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Launcher_lua PRIVATE m)
endif()
