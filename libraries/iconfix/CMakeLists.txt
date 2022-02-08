cmake_minimum_required(VERSION 3.1)
project(iconfix)

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5Widgets REQUIRED QUIET)

set(ICONFIX_SOURCES
xdgicon.h
xdgicon.cpp
internal/qhexstring_p.h
internal/qiconloader.cpp
internal/qiconloader_p.h
)

add_library(Launcher_iconfix ${ICONFIX_SOURCES})
target_include_directories(Launcher_iconfix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}" )

target_link_libraries(Launcher_iconfix Qt5::Core Qt5::Widgets)

generate_export_header(Launcher_iconfix)
