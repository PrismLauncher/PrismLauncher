cmake_minimum_required(VERSION 3.1)

project(MultiMC_unpack200)

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

if (Qt5_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_library(MultiMC_unpack200 SHARED ${PACK200_SRC})
target_include_directories(MultiMC_unpack200 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}" PRIVATE ${ZLIB_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(MultiMC_unpack200 ${ZLIB_LIBRARIES})

set_target_properties(MultiMC_unpack200 PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
generate_export_header(MultiMC_unpack200)

# Install it
install(
    TARGETS MultiMC_unpack200
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)

if(PACK200_BUILD_BINARY)
    add_executable(anti200 anti200.cpp)
    target_link_libraries(anti200 MultiMC_unpack200)
endif()
