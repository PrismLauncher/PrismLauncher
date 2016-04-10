cmake_minimum_required(VERSION 3.1)

project(unpack200)

option(PACK200_BUILD_BINARY "Build a tiny utility that decompresses pack200 streams" OFF)

# Find ZLIB for quazip
find_package(ZLIB REQUIRED)

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

add_library(unpack200 STATIC ${PACK200_SRC})
target_include_directories(unpack200 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE ${ZLIB_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(unpack200 ${ZLIB_LIBRARIES})

if(PACK200_BUILD_BINARY)
	add_executable(anti200 anti200.cpp)
	target_link_libraries(anti200 unpack200)
endif()
