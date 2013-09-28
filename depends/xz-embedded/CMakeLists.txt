cmake_minimum_required(VERSION 2.6)
project(xz-embedded)

option(XZ_BUILD_BCJ "Build xz-embedded with BCJ support (native binary optimization)" OFF)
option(XZ_BUILD_CRC64 "Build xz-embedded with CRC64 checksum support" ON)
option(XZ_BUILD_MINIDEC "Build a tiny utility that decompresses xz streams" ON)

set(CMAKE_C_FLAGS "-std=c99")

include_directories(include)

set(XZ_SOURCES
include/xz.h
src/xz_config.h
src/xz_crc32.c
src/xz_dec_lzma2.c
src/xz_dec_stream.c
src/xz_lzma2.h
src/xz_private.h
src/xz_stream.h
)

# checksum checks
add_definitions(-DXZ_DEC_ANY_CHECK)
if(XZ_BUILD_CRC64)
	add_definitions(-DXZ_USE_CRC64)
	LIST(APPEND XZ_SOURCES src/xz_crc64.c)
endif()
# TODO: add SHA256

# uncomment these,  if required.
if(XZ_BUILD_BCJ)
	add_definitions(-DXZ_DEC_X86 -DXZ_DEC_POWERPC -DXZ_DEC_IA64)
	add_definitions(-DXZ_DEC_ARM -DXZ_DEC_ARMTHUMB -DXZ_DEC_SPARC)
	LIST(APPEND XZ_SOURCES src/xz_dec_bcj.c)
endif()

# Static link!
ADD_DEFINITIONS(-DXZ_STATIC)

add_definitions(-DXZ_LIBRARY)

add_library(xz-embedded SHARED ${XZ_SOURCES})
add_executable(xzminidec xzminidec.c)
target_link_libraries(xzminidec xz-embedded)
