cmake_minimum_required(VERSION 3.1)
project(LocalPeer)

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5Network REQUIRED QUIET)

set(SINGLE_SOURCES
src/LocalPeer.cpp
src/LockedFile.cpp
src/LockedFile.h
include/LocalPeer.h
)

if(UNIX)
	list(APPEND SINGLE_SOURCES
		src/LockedFile_unix.cpp
	)
endif()

if(WIN32)
	list(APPEND SINGLE_SOURCES
		src/LockedFile_win.cpp
	)
endif()

add_library(LocalPeer STATIC ${SINGLE_SOURCES})
target_include_directories(LocalPeer PUBLIC include)

target_link_libraries(LocalPeer Qt5::Core Qt5::Network)
