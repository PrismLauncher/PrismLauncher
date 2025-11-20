cmake_minimum_required(VERSION 3.10)
project(dlopen_hook)

set(CMAKE_C_STANDARD 11)

set(SRC
    inject.c
)

add_library(dlopen_hook SHARED ${SRC})

# Look in same path that Java executable would
set_target_properties(dlopen_hook PROPERTIES
    INSTALL_RPATH "@executable_path/../lib;@executable_path/../jre/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

target_link_libraries(dlopen_hook
    "-framework Foundation"
)
