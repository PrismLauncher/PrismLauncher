cmake_minimum_required(VERSION 3.6)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BUILD_DIR}" IS_IN_SOURCE_BUILD)
if(IS_IN_SOURCE_BUILD)
    message(FATAL_ERROR "You are building Katabasis in-source. Please separate the build tree from the source tree.")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_HOST_SYSTEM_VERSION MATCHES ".*[Mm]icrosoft.*" OR
        CMAKE_HOST_SYSTEM_VERSION MATCHES ".*WSL.*"
    )
        message(FATAL_ERROR "Building Katabasis is not supported in Linux-on-Windows distributions. Use a real Linux machine, not a fraudulent one.")
    endif()
endif()

project(Katabasis)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

set( katabasis_PRIVATE
    src/OAuth2.cpp
    src/JsonResponse.cpp
    src/JsonResponse.h
    src/PollServer.cpp
    src/Reply.cpp
    src/ReplyServer.cpp
)

set( katabasis_PUBLIC
    include/katabasis/OAuth2.h
    include/katabasis/Globals.h
    include/katabasis/PollServer.h
    include/katabasis/Reply.h
    include/katabasis/ReplyServer.h
    include/katabasis/RequestParameter.h
)

add_library( Katabasis STATIC ${katabasis_PRIVATE} ${katabasis_PUBLIC} )
target_link_libraries(Katabasis Qt5::Core Qt5::Network)

# needed for statically linked Katabasis in shared libs on x86_64
set_target_properties(Katabasis
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE
)

target_include_directories(Katabasis PUBLIC include PRIVATE src include/katabasis)
