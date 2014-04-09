cmake_minimum_required(VERSION 2.6)
project(xz-embedded)

option(XZ_BUILD_BCJ "Build xz-embedded with BCJ support (native binary optimization)" OFF)
option(XZ_BUILD_CRC64 "Build xz-embedded with CRC64 checksum support" ON)
option(XZ_BUILD_MINIDEC "Build a tiny utility that decompresses xz streams" OFF)

set(CMAKE_C_FLAGS "-std=c99")

include_directories(include)
set(XZ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# See include/xz.h for manual feature configuration
# tweak this list and xz.h to fit your needs

set(XZ_SOURCES
	include/xz.h
	src/xz_config.h
	src/xz_crc32.c
	src/xz_crc64.c
	src/xz_dec_lzma2.c
	src/xz_dec_stream.c
	src/xz_lzma2.h
	src/xz_private.h
	src/xz_stream.h
#	src/xz_dec_bcj.c
)
# TODO: look into what would be needed for plain old lzma

add_library(xz-embedded STATIC ${XZ_SOURCES})
add_executable(xzminidec xzminidec.c)
target_link_libraries(xzminidec xz-embedded)
