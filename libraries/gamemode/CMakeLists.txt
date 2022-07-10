cmake_minimum_required(VERSION 3.9.4)
project(gamemode
    VERSION 1.6.1)

add_library(gamemode)
target_include_directories(gamemode PUBLIC include)
target_link_libraries(gamemode PUBLIC ${CMAKE_DL_LIBS})
