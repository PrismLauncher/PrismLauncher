CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
cmake_policy(SET CMP0020 OLD)

project(unpack200)

# Find ZLIB for quazip
# Use system zlib on unix and Qt ZLIB on Windows
IF(UNIX)
	find_package(ZLIB REQUIRED)
ELSE(UNIX)
	get_filename_component (ZLIB_FOUND_DIR "${Qt5Core_DIR}/../../../include/QtZlib" ABSOLUTE)
	SET(ZLIB_INCLUDE_DIRS ${ZLIB_FOUND_DIR} CACHE PATH "Path to ZLIB headers of Qt")
	SET(ZLIB_LIBRARIES "")
	IF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
		MESSAGE("Please specify a valid zlib include dir")
	ENDIF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
ENDIF(UNIX)

SET(PACK200_SRC
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

SET(PACK200_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
include_directories(
	include
	${ZLIB_INCLUDE_DIRS}
)
add_library(unpack200 STATIC ${PACK200_SRC})

IF(UNIX)
	target_link_libraries(unpack200 ${ZLIB_LIBRARIES})
ELSE()
	# zlib is part of Qt on windows. use it.
	QT5_USE_MODULES(unpack200 Core)
ENDIF()

add_executable(anti200 anti200.cpp)
target_link_libraries(anti200 unpack200)
