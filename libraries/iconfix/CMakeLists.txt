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

add_library(iconfix STATIC ${ICONFIX_SOURCES})
target_include_directories(iconfix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt5_use_modules(iconfix Core Widgets)
