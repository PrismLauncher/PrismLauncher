cmake_minimum_required(VERSION 3.1)
project(xz-embedded LANGUAGES C)

option(XZ_BUILD_BCJ "Build xz-embedded with BCJ support (native binary optimization)" OFF)
option(XZ_BUILD_CRC64 "Build xz-embedded with CRC64 checksum support" ON)
option(XZ_BUILD_MINIDEC "Build a tiny utility that decompresses xz streams" OFF)

# See include/xz.h for manual feature configuration
# tweak this list and xz.h to fit your needs

set(XZ_SOURCES
	src/xz_crc32.c
	src/xz_crc64.c
	src/xz_dec_lzma2.c
	src/xz_dec_stream.c
#	src/xz_dec_bcj.c
)
add_library(xz-embedded STATIC ${XZ_SOURCES})
target_include_directories(xz-embedded PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_property(TARGET xz-embedded PROPERTY C_STANDARD 99)

if(${XZ_BUILD_MINIDEC})
	add_executable(xzminidec xzminidec.c)
	target_link_libraries(xzminidec xz-embedded)
	set_property(TARGET xzminidec PROPERTY C_STANDARD 99)
endif()
