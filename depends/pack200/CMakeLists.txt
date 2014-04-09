cmake_minimum_required(VERSION 2.6)

if(WIN32)
	# In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
	cmake_policy(SET CMP0020 OLD)
endif()

project(unpack200)

# Find ZLIB for quazip
# Use system zlib on unix and Qt ZLIB on Windows
if(UNIX)
	find_package(ZLIB REQUIRED)
else(UNIX)
	get_filename_component(ZLIB_FOUND_DIR "${Qt5Core_DIR}/../../../include/QtZlib" ABSOLUTE)
	set(ZLIB_INCLUDE_DIRS ${ZLIB_FOUND_DIR} CACHE PATH "Path to ZLIB headers of Qt")
	set(ZLIB_LIBRARIES "")
	if(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
		message("Please specify a valid zlib include dir")
	endif(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
endif(UNIX)

set(PACK200_SRC
	include/unpack200.h
	src/bands.cpp
	src/bands.h
	src/bytes.cpp
	src/bytes.h
	src/coding.cpp
	src/coding.h
	src/constants.h
	src/defines.h
	src/unpack200.cpp
	src/unpack.cpp
	src/unpack.h
	src/utils.cpp
	src/utils.h
	src/zip.cpp
	src/zip.h
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PACK200_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
include_directories(
	include
	${ZLIB_INCLUDE_DIRS}
)
add_library(unpack200 STATIC ${PACK200_SRC})

if(UNIX)
	target_link_libraries(unpack200 ${ZLIB_LIBRARIES})
else()
	# zlib is part of Qt on windows. use it.
	qt5_use_modules(unpack200 Core)
endif()

add_executable(anti200 anti200.cpp)
target_link_libraries(anti200 unpack200)
