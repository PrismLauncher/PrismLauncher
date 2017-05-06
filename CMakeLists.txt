cmake_minimum_required(VERSION 3.1)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BUILD_DIR}" IS_IN_SOURCE_BUILD)
if(IS_IN_SOURCE_BUILD)
	message(AUTHOR_WARNING "You are building MultiMC in-source. This is NOT recommended!")
endif()

if(WIN32)
	# In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
	cmake_policy(SET CMP0020 OLD)
endif()

project(MultiMC)
enable_testing()

######## Set CMake options ########
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(UNIX)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
include(GenerateExportHeader)
set(CMAKE_CXX_FLAGS " -Wall -D_GLIBCXX_USE_CXX11_ABI=0 ${CMAKE_CXX_FLAGS}")
if(UNIX AND APPLE)
	set(CMAKE_CXX_FLAGS " -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")


################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Concurrent)
find_package(Qt5Network)
find_package(Qt5Test)
find_package(Qt5Xml)

# The Qt5 cmake files don't provide its install paths, so ask qmake.
include(QMakeQuery)
query_qmake(QT_INSTALL_PLUGINS QT_PLUGINS_DIR)
query_qmake(QT_INSTALL_IMPORTS QT_IMPORTS_DIR)
query_qmake(QT_INSTALL_LIBS QT_LIBS_DIR)
query_qmake(QT_INSTALL_LIBEXECS QT_LIBEXECS_DIR)
query_qmake(QT_HOST_DATA QT_DATA_DIR)
set(QT_MKSPECS_DIR ${QT_DATA_DIR}/mkspecs)

if (Qt5_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

option(NBT_BUILD_SHARED "Build NBT shared library" ON)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
set(NBT_NAME MultiMC_nbt++)
add_subdirectory(libraries/libnbtplusplus)

add_subdirectory(libraries/ganalytics) # google analytics library
add_subdirectory(libraries/systeminfo) # system information library
add_subdirectory(libraries/hoedown) # markdown parser
add_subdirectory(libraries/launcher) # java based launcher part for Minecraft
add_subdirectory(libraries/javacheck) # java compatibility checker
add_subdirectory(libraries/xz-embedded) # xz compression
add_subdirectory(libraries/quazip) # zip manipulation library
add_subdirectory(libraries/pack200) # java pack200 compression
add_subdirectory(libraries/rainbow) # Qt extension for colors
add_subdirectory(libraries/iconfix) # fork of Qt's QIcon loader
add_subdirectory(libraries/LocalPeer) # fork of a library from Qt solutions

############################### Built Artifacts ###############################

add_subdirectory(api/logic)
add_subdirectory(api/gui)

add_subdirectory(application)
