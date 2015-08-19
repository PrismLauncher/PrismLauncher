cmake_minimum_required(VERSION 2.8.11)
project(rainbow)

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5Gui REQUIRED QUIET)

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})

set(RAINBOW_SOURCES
include/rainbow_config.h
include/rainbow.h
src/rainbow.cpp
)

add_definitions(-DRAINBOW_LIBRARY)
set(RAINBOW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
add_library(rainbow SHARED ${RAINBOW_SOURCES})

qt5_use_modules(rainbow Core Gui)
