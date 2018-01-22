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

add_library(MultiMC_iconfix SHARED ${ICONFIX_SOURCES})
target_include_directories(MultiMC_iconfix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}" )

qt5_use_modules(MultiMC_iconfix Core Widgets)

set_target_properties(MultiMC_iconfix PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
generate_export_header(MultiMC_iconfix)

# Install it
install(TARGETS MultiMC_iconfix DESTINATION ${LIBRARY_DEST_DIR})
