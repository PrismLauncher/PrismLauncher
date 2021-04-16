cmake_minimum_required(VERSION 3.1)
project(toml11)

add_library(toml11 INTERFACE)
target_include_directories(toml11 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
