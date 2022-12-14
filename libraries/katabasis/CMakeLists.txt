cmake_minimum_required(VERSION 3.9.4)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BUILD_DIR}" IS_IN_SOURCE_BUILD)
if(IS_IN_SOURCE_BUILD)
    message(FATAL_ERROR "You are building Katabasis in-source. Please separate the build tree from the source tree.")
endif()

project(Katabasis)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 COMPONENTS Core Network REQUIRED)
elseif(Launcher_QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Core Network REQUIRED)
endif()

set( katabasis_PRIVATE
    src/DeviceFlow.cpp
    src/JsonResponse.cpp
    src/JsonResponse.h
    src/PollServer.cpp
    src/Reply.cpp
)

set( katabasis_PUBLIC
    include/katabasis/DeviceFlow.h
    include/katabasis/Globals.h
    include/katabasis/PollServer.h
    include/katabasis/Reply.h
    include/katabasis/RequestParameter.h
)

ecm_qt_declare_logging_category(katabasis_PRIVATE
    HEADER KatabasisLogging.h  # NOTE: this won't be in src/, but CMAKE_BINARY_DIR/src isn't included by default so this should be fine
    IDENTIFIER katabasisCredentials
    CATEGORY_NAME "katabasis.credentials"
    DEFAULT_SEVERITY Warning
    DESCRIPTION "Secrets and credentials from Katabasis"
    EXPORT "Katabasis"
)

add_library( Katabasis STATIC ${katabasis_PRIVATE} ${katabasis_PUBLIC} )
target_link_libraries(Katabasis Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

# needed for statically linked Katabasis in shared libs on x86_64
set_target_properties(Katabasis
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE
)

target_include_directories(Katabasis PUBLIC include PRIVATE src include/katabasis)
