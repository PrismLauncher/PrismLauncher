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

######## Set module path ########
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

# Only used for test coverage
set(MMC_SRC "${PROJECT_SOURCE_DIR}")
set(MMC_BIN "${PROJECT_BINARY_DIR}")

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
include(Coverage)
include(GenerateExportHeader)
set(CMAKE_CXX_FLAGS " -Wall -D_GLIBCXX_USE_CXX11_ABI=0 ${CMAKE_CXX_FLAGS}")
if(UNIX AND APPLE)
	set(CMAKE_CXX_FLAGS " -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")

# cmake code needed for the coverity scan upload
include(Coverity)

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

# Add quazip
add_definitions(-DQUAZIP_STATIC)
set(QUAZIP_VERSION "0.7.1")
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz)
	file(DOWNLOAD http://downloads.sourceforge.net/project/quazip/quazip/${QUAZIP_VERSION}/quazip-${QUAZIP_VERSION}.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz)
endif()
ExternalProject_Add(QuaZIP
	SOURCE_DIR <BINARY_DIR>/../Source/quazip-${QUAZIP_VERSION}
	DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/.. ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz
	PATCH_COMMAND patch -p0 -i ${CMAKE_SOURCE_DIR}/quazip.patch
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/External/Install/QuaZIP/include/quazip")
if(UNIX)
set(QUAZIP_LIBRARIES -L"${CMAKE_CURRENT_BINARY_DIR}/External/Install/QuaZIP/lib" quazip z)
else()
set(QUAZIP_LIBRARIES -L"${CMAKE_CURRENT_BINARY_DIR}/External/Install/QuaZIP/lib" quazip)
endif()

add_subdirectory(depends/hoedown) # markdown parser
add_subdirectory(depends/launcher) # java based launcher part for Minecraft
add_subdirectory(depends/javacheck) # java compatibility checker
add_subdirectory(depends/xz-embedded) # xz compression
add_subdirectory(depends/pack200) # java pack200 compression
add_subdirectory(depends/rainbow) # Qt extension for colors

option(NBT_BUILD_SHARED "Build NBT shared library" ON)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
add_subdirectory(depends/libnbtplusplus)

######## MultiMC Libs ########

add_subdirectory(depends/LogicalGui) # GUI -> Logic connection
add_subdirectory(depends/iconfix) # fork of Qt's QIcon loader

include(Coverity)

############################### Built Artifacts ###############################

add_subdirectory(tests)
add_subdirectory(libraries/logic)
add_subdirectory(libraries/gui)

add_subdirectory(application)
add_subdirectory(wonkoclient)
