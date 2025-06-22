cmake_minimum_required(VERSION 3.15)
project(murmur2)

set(MURMUR_SOURCES
    src/MurmurHash2.h
    src/MurmurHash2.cpp
)

add_library(Launcher_murmur2 STATIC ${MURMUR_SOURCES})
target_include_directories(Launcher_murmur2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "src" )

generate_export_header(Launcher_murmur2)
