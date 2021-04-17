project(tomlc99)

set(tomlc99_SOURCES
include/toml.h
src/toml.c
)

add_library(tomlc99 STATIC ${tomlc99_SOURCES})

target_include_directories(tomlc99 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
