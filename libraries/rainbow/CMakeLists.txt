cmake_minimum_required(VERSION 3.1)
project(rainbow)

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5Gui REQUIRED QUIET)

set(RAINBOW_SOURCES
src/rainbow.cpp
)

add_library(Launcher_rainbow STATIC ${RAINBOW_SOURCES})
target_include_directories(Launcher_rainbow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(Launcher_rainbow Qt5::Core Qt5::Gui)
