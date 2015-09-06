enable_testing()
include_directories(${libnbt++_SOURCE_DIR}/include)
include_directories(${CXXTEST_INCLUDE_DIR})

CXXTEST_ADD_TEST(nbttest nbttest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/nbttest.h)
target_link_libraries(nbttest nbt++)

CXXTEST_ADD_TEST(endian_str_test endian_str_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/endian_str_test.h)
target_link_libraries(endian_str_test nbt++)

CXXTEST_ADD_TEST(read_test read_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/read_test.h)
target_link_libraries(read_test nbt++)
add_custom_command(TARGET read_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/testfiles ${CMAKE_CURRENT_BINARY_DIR})

CXXTEST_ADD_TEST(write_test write_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/write_test.h)
target_link_libraries(write_test nbt++)

add_executable(format_test format_test.cpp)
target_link_libraries(format_test nbt++)
add_test(format_test format_test)
