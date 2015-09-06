cmake_minimum_required(VERSION 3.2)
project(libnbt++ VERSION 2.1)

add_definitions(-std=c++11)
include_directories(include)

set(nbt_sources
	src/endian_str.cpp
	src/tag.cpp
	src/tag_array.cpp
	src/tag_compound.cpp
	src/tag_list.cpp
	src/tag_string.cpp
	src/value.cpp
	src/value_initializer.cpp

	src/io/stream_reader.cpp
	src/io/stream_writer.cpp

	src/text/json_formatter.cpp

	include/value_initializer.h
	include/tag.h
	include/io
	include/io/stream_writer.h
	include/io/stream_reader.h
	include/crtp_tag.h
	include/tag_string.h
	include/value.h
	include/tag_primitive.h
	include/tag_list.h
	include/tagfwd.h
	include/make_unique.h
	include/primitive_detail.h
	include/endian_str.h
	include/tag_compound.h
	include/nbt_tags.h
	include/nbt_visitor.h
	include/text
	include/text/json_formatter.h
	include/tag_array.h
)

set(LIBNBTPP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
set(LIBNBTPP_BIN_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

add_library(nbt++ SHARED ${nbt_sources})
generate_export_header(nbt++)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
