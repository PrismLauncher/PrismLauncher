cmake_minimum_required(VERSION 2.8.11)
project(iconfix)

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5Widgets REQUIRED QUIET)

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

set(ICONFIX_SOURCES
xdgicon.h
xdgicon.cpp
internal/qhexstring_p.h
internal/qiconloader.cpp
internal/qiconloader_p.h
)

set(ICONFIX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
add_library(iconfix STATIC ${ICONFIX_SOURCES})
qt5_use_modules(iconfix Core Widgets)
