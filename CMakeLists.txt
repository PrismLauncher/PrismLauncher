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

##################################### Set CMake options #####################################
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
set(CMAKE_CXX_FLAGS " -Wall -pedantic -Werror -D_GLIBCXX_USE_CXX11_ABI=0 -fstack-protector-strong --param=ssp-buffer-size=4 -O3 -D_FORTIFY_SOURCE=2 ${CMAKE_CXX_FLAGS}")
if(UNIX AND APPLE)
    set(CMAKE_CXX_FLAGS " -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")

##################################### Set Application options #####################################

######## Set URLs ########
set(MultiMC_NEWS_RSS_URL "https://multimc.org/rss.xml" CACHE STRING "URL to fetch MultiMC's news RSS feed from.")

######## Set version numbers ########
set(MultiMC_VERSION_MAJOR    0)
set(MultiMC_VERSION_MINOR    6)
set(MultiMC_VERSION_HOTFIX   4)

# Build number
set(MultiMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Build platform.
set(MultiMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Channel list URL
set(MultiMC_CHANLIST_URL "" CACHE STRING "URL for the channel list.")

# Notification URL
set(MultiMC_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

# paste.ee API key
set(MultiMC_PASTE_EE_API_KEY "utLvciUouSURFzfjPxLBf5W4ISsUX4pwBDF7N1AfZ" CACHE STRING "API key you can get from paste.ee when you register an account")

# Google analytics ID
set(MultiMC_ANALYTICS_ID "UA-87731965-2" CACHE STRING "ID you can get from Google analytics")

#### Check the current Git commit and branch
include(GetGitRevisionDescription)
get_git_head_revision(MultiMC_GIT_REFSPEC MultiMC_GIT_COMMIT)

message(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")
message(STATUS "Git refspec: ${MultiMC_GIT_REFSPEC}")

set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}")

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MultiMC_RELEASE_VERSION_NAME}")

################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Xml REQUIRED)

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

####################################### Install layout #######################################

# How to install the build results
set(MultiMC_LAYOUT "auto" CACHE STRING "The layout for MultiMC installation (auto, win-bundle, lin-bundle, lin-nodeps, lin-system, mac-bundle)")
set_property(CACHE MultiMC_LAYOUT PROPERTY STRINGS auto win-bundle lin-bundle lin-nodeps lin-system mac-bundle)

if(MultiMC_LAYOUT STREQUAL "auto")
    if(UNIX AND APPLE)
        set(MultiMC_LAYOUT_REAL "mac-bundle")
    elseif(UNIX)
        set(MultiMC_LAYOUT_REAL "lin-nodeps")
    elseif(WIN32)
        set(MultiMC_LAYOUT_REAL "win-bundle")
    else()
        message(FATAL_ERROR "Cannot choose a sensible install layout for your platform.")
    endif()
else()
    set(MultiMC_LAYOUT_REAL ${MultiMC_LAYOUT})
endif()

if(MultiMC_LAYOUT_REAL STREQUAL "mac-bundle")
    set(BINARY_DEST_DIR "MultiMC.app/Contents/MacOS")
    set(LIBRARY_DEST_DIR "MultiMC.app/Contents/MacOS")
    set(PLUGIN_DEST_DIR "MultiMC.app/Contents/MacOS")
    set(RESOURCES_DEST_DIR "MultiMC.app/Contents/Resources")
    set(JARS_DEST_DIR "MultiMC.app/Contents/MacOS/jars")

    set(BUNDLE_DEST_DIR ".")

    # Apps to bundle
    set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")

    # Mac bundle settings
    set(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
    set(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.multimc.MultiMC5")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
    set(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2015-2019 MultiMC Contributors")

    # directories to look for dependencies
    set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    # install as bundle
    set(INSTALL_BUNDLE "full")

    # Add the icon
    install(FILES application/resources/MultiMC.icns DESTINATION ${RESOURCES_DEST_DIR})

elseif(MultiMC_LAYOUT_REAL STREQUAL "lin-bundle")
    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "bin")
    set(PLUGIN_DEST_DIR "plugins")
    set(BUNDLE_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")
    set(JARS_DEST_DIR "bin/jars")

    # Apps to bundle
    set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")

    # directories to look for dependencies
    set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    # install as bundle
    set(INSTALL_BUNDLE "full")

    # Set RPATH
    SET(MultiMC_BINARY_RPATH "$ORIGIN/")

    # Install basic runner script
    install(PROGRAMS application/package/linux/MultiMC DESTINATION ${BUNDLE_DEST_DIR})

elseif(MultiMC_LAYOUT_REAL STREQUAL "lin-nodeps")
    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "bin")
    set(PLUGIN_DEST_DIR "plugins")
    set(BUNDLE_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")
    set(JARS_DEST_DIR "bin/jars")

    # install as bundle with no dependencies included
    set(INSTALL_BUNDLE "nodeps")

    # Set RPATH
    SET(MultiMC_BINARY_RPATH "$ORIGIN/")

    # Install basic runner script
    install(PROGRAMS application/package/linux/MultiMC DESTINATION ${BUNDLE_DEST_DIR})

elseif(MultiMC_LAYOUT_REAL STREQUAL "lin-system")
    set(MultiMC_APP_BINARY_NAME "multimc" CACHE STRING "Name of the MultiMC binary")
    set(MultiMC_BINARY_DEST_DIR "bin" CACHE STRING "Path to the binary directory")
    set(MultiMC_LIBRARY_DEST_DIR "lib${LIB_SUFFIX}" CACHE STRING "Path to the library directory")
    set(MultiMC_SHARE_DEST_DIR "share/multimc" CACHE STRING "Path to the shared data directory")
    set(JARS_DEST_DIR "${MultiMC_SHARE_DEST_DIR}/jars")

    set(BINARY_DEST_DIR ${MultiMC_BINARY_DEST_DIR})
    set(LIBRARY_DEST_DIR ${MultiMC_LIBRARY_DEST_DIR})

    MESSAGE(STATUS "Compiling for linux system with ${MultiMC_SHARE_DEST_DIR} and MULTIMC_LINUX_DATADIR")
    SET(MultiMC_APP_BINARY_DEFS "-DMULTIMC_JARS_LOCATION=${CMAKE_INSTALL_PREFIX}/${JARS_DEST_DIR}" "-DMULTIMC_LINUX_DATADIR")

    # install as bundle with no dependencies included
    set(INSTALL_BUNDLE "nodeps")

elseif(MultiMC_LAYOUT_REAL STREQUAL "win-bundle")
    set(BINARY_DEST_DIR ".")
    set(LIBRARY_DEST_DIR ".")
    set(PLUGIN_DEST_DIR ".")
    set(BUNDLE_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")
    set(JARS_DEST_DIR "jars")

    # Apps to bundle
    set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.exe")

    # directories to look for dependencies
    set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    # install as bundle
    set(INSTALL_BUNDLE "full")
else()
    message(FATAL_ERROR "No sensible install layout set.")
endif()

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

option(NBT_BUILD_SHARED "Build NBT shared library" ON)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
set(NBT_NAME MultiMC_nbt++)
set(NBT_DEST_DIR ${LIBRARY_DEST_DIR})
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
add_subdirectory(libraries/classparser) # google analytics library

############################### Built Artifacts ###############################

add_subdirectory(api/logic)
add_subdirectory(api/gui)

# NOTE: this must always be last to appease the CMake deity of quirky install command evaluation order.
add_subdirectory(application)
