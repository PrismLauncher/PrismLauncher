cmake_minimum_required(VERSION 3.15)
project(qdcss)

if(Launcher_QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Core REQUIRED)
endif()

set(QDCSS_SOURCES
    include/qdcss.h
    src/qdcss.cpp
)

add_library(qdcss STATIC ${QDCSS_SOURCES})
target_include_directories(qdcss PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(qdcss Qt${QT_VERSION_MAJOR}::Core ${qdcss_LIBS})
