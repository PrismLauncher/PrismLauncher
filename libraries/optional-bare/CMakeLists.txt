cmake_minimum_required(VERSION 3.9.4)
project(optional-bare)

add_library(optional-bare INTERFACE)
target_include_directories(optional-bare INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
