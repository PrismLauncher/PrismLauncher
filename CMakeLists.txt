cmake_minimum_required(VERSION 3.15)  # minimum version required by QuaZip

if(WIN32)
    # In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
    cmake_policy(SET CMP0020 OLD)
endif()

project(Launcher)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BUILD_DIR}" IS_IN_SOURCE_BUILD)
if(IS_IN_SOURCE_BUILD)
    message(FATAL_ERROR "You are building the Launcher in-source. Please separate the build tree from the source tree.")
endif()

##################################### Set CMake options #####################################
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
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
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
include(GenerateExportHeader)
set(CMAKE_CXX_FLAGS "-Wall -pedantic -fstack-protector-strong --param=ssp-buffer-size=4 ${CMAKE_CXX_FLAGS}")

# Fix build with Qt 5.13
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_DEPRECATED_WARNINGS=Y")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_DISABLE_DEPRECATED_BEFORE=0x050C00")

# set CXXFLAGS for build targets
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -D_FORTIFY_SOURCE=2 ${CMAKE_CXX_FLAGS_RELEASE}")

option(ENABLE_LTO "Enable Link Time Optimization" off)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel"))
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    elseif(ipo_supported)
        message(STATUS "Not enabling IPO / LTO on debug builds")
    else()
        message(STATUS "IPO / LTO not supported: <${ipo_error}>")
    endif()
endif()

option(BUILD_TESTING "Build the testing tree." ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH};${CMAKE_MODULE_PATH}")
include(CTest)
include(ECMAddTests)
if(BUILD_TESTING)
    enable_testing()
endif()

##################################### Set Application options #####################################

######## Set URLs ########
set(Launcher_NEWS_RSS_URL "https://prismlauncher.org/feed/feed.xml" CACHE STRING "URL to fetch Prism Launcher's news RSS feed from.")
set(Launcher_NEWS_OPEN_URL "https://prismlauncher.org/news" CACHE STRING "URL that gets opened when the user clicks 'More News'")
set(Launcher_HELP_URL "https://prismlauncher.org/wiki/help-pages/%1" CACHE STRING "URL (with arg %1 to be substituted with page-id) that gets opened when the user requests help")

######## Set version numbers ########
set(Launcher_VERSION_MAJOR 5)
set(Launcher_VERSION_MINOR 0)

set(Launcher_VERSION_NAME "${Launcher_VERSION_MAJOR}.${Launcher_VERSION_MINOR}")
set(Launcher_VERSION_NAME4 "${Launcher_VERSION_MAJOR}.${Launcher_VERSION_MINOR}.0.0")
set(Launcher_VERSION_NAME4_COMMA "${Launcher_VERSION_MAJOR},${Launcher_VERSION_MINOR},0,0")

# Build platform.
set(Launcher_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used to display in the about dialog.")

# Channel list URL
set(Launcher_UPDATER_BASE "" CACHE STRING "Base URL for the updater.")

# The metadata server
set(Launcher_META_URL "https://meta.prismlauncher.org/v1/" CACHE STRING "URL to fetch Launcher's meta files from.")

# Imgur API Client ID
set(Launcher_IMGUR_CLIENT_ID "5b97b0713fba4a3" CACHE STRING "Client ID you can get from Imgur when you register an application")

# Bug tracker URL
set(Launcher_BUG_TRACKER_URL "https://github.com/PrismLauncher/PrismLauncher/issues" CACHE STRING "URL for the bug tracker.")

# Translations Platform URL
set(Launcher_TRANSLATIONS_URL "https://hosted.weblate.org/projects/prismlauncher/launcher/" CACHE STRING "URL for the translations platform.")

# Matrix Space
set(Launcher_MATRIX_URL "https://matrix.to/#/#prismlauncher:matrix.org" CACHE STRING "URL to the Matrix Space")

# Discord URL
set(Launcher_DISCORD_URL "https://discord.gg/prismlauncher" CACHE STRING "URL for the Discord guild.")

# Subreddit URL
set(Launcher_SUBREDDIT_URL "https://www.reddit.com/r/PrismLauncher/" CACHE STRING "URL for the subreddit.")

# Builds
set(Launcher_FORCE_BUNDLED_LIBS OFF CACHE BOOL "Prevent using system libraries, if they are available as submodules")
set(Launcher_QT_VERSION_MAJOR "5" CACHE STRING "Major Qt version to build against")

# API Keys
# NOTE: These API keys are here for convenience. If you rebrand this software or intend to break the terms of service
# of these platforms, please change these API keys beforehand.
# Be aware that if you were to use these API keys for malicious purposes they might get revoked, which might cause
# breakage to thousands of users.
# If you don't plan to use these features of this software, you can just remove these values.

# By using this key in your builds you accept the terms of use laid down in
# https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use
set(Launcher_MSA_CLIENT_ID "c36a9fb6-4f2a-41ff-90bd-ae7cc92031eb" CACHE STRING "Client ID you can get from Microsoft Identity Platform when you register an application")

# By using this key in your builds you accept the terms and conditions laid down in
# https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions
# NOTE: CurseForge requires you to change this if you make any kind of derivative work.
# This key was issued specifically for Prism Launcher
set(Launcher_CURSEFORGE_API_KEY "$2a$10$wuAJuNZuted3NORVmpgUC.m8sI.pv1tOPKZyBgLFGjxFp/br0lZCC" CACHE STRING "API key for the CurseForge platform")


#### Check the current Git commit and branch
include(GetGitRevisionDescription)
git_get_exact_tag(Launcher_GIT_TAG)
get_git_head_revision(Launcher_GIT_REFSPEC Launcher_GIT_COMMIT)

message(STATUS "Git commit: ${Launcher_GIT_COMMIT}")
message(STATUS "Git tag: ${Launcher_GIT_TAG}")
message(STATUS "Git refspec: ${Launcher_GIT_REFSPEC}")

string(TIMESTAMP TODAY "%Y-%m-%d")
set(Launcher_BUILD_TIMESTAMP "${TODAY}")

################################ 3rd Party Libs ################################

# Find the required Qt parts
include(QtVersionlessBackport)
if(Launcher_QT_VERSION_MAJOR EQUAL 5)
    set(QT_VERSION_MAJOR 5)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Network Test Xml)

    if(NOT Launcher_FORCE_BUNDLED_LIBS)
        find_package(QuaZip-Qt5 1.3 QUIET)
    endif()
    if (NOT QuaZip-Qt5_FOUND)
        set(QUAZIP_QT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "Qt version to use (4, 5 or 6), defaults to ${QT_VERSION_MAJOR}" FORCE)
        set(FORCE_BUNDLED_QUAZIP 1)
    endif()

    # Qt 6 sets these by default. Notably causes Windows APIs to use UNICODE strings.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
elseif(Launcher_QT_VERSION_MAJOR EQUAL 6)
    set(QT_VERSION_MAJOR 6)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Test Xml Core5Compat)
    list(APPEND Launcher_QT_LIBS Qt6::Core5Compat)

    if(NOT Launcher_FORCE_BUNDLED_LIBS)
        find_package(QuaZip-Qt6 1.3 QUIET)
    endif()
    if (NOT QuaZip-Qt6_FOUND)
        set(QUAZIP_QT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "Qt version to use (4, 5 or 6), defaults to ${QT_VERSION_MAJOR}" FORCE)
        set(FORCE_BUNDLED_QUAZIP 1)
    endif()
else()
    message(FATAL_ERROR "Qt version ${Launcher_QT_VERSION_MAJOR} is not supported")
endif()

include(ECMQueryQt)
ecm_query_qt(QT_PLUGINS_DIR  QT_INSTALL_PLUGINS)
ecm_query_qt(QT_LIBS_DIR     QT_INSTALL_LIBS)
ecm_query_qt(QT_LIBEXECS_DIR QT_INSTALL_LIBEXECS)
ecm_query_qt(QT_DATA_DIR     QT_HOST_DATA)
set(QT_MKSPECS_DIR ${QT_DATA_DIR}/mkspecs)

# NOTE: Qt 6 already sets this by default
if (Qt5_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(NOT Launcher_FORCE_BUNDLED_LIBS)
    # Find toml++
    find_package(tomlplusplus 3.2.0 QUIET)

    # Find ghc_filesystem
    find_package(ghc_filesystem QUIET)
endif()

####################################### Program Info #######################################

set(Launcher_APP_BINARY_NAME "prismlauncher" CACHE STRING "Name of the Launcher binary")
add_subdirectory(program_info)

####################################### Install layout #######################################

if(NOT (UNIX AND APPLE))
    # Install "portable.txt" if selected component is "portable"
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_Portable_File}" DESTINATION "." COMPONENT portable EXCLUDE_FROM_ALL)
endif()

if(UNIX AND APPLE)
    set(BINARY_DEST_DIR "${Launcher_Name}.app/Contents/MacOS")
    set(LIBRARY_DEST_DIR "${Launcher_Name}.app/Contents/MacOS")
    set(PLUGIN_DEST_DIR "${Launcher_Name}.app/Contents/MacOS")
    set(FRAMEWORK_DEST_DIR "${Launcher_Name}.app/Contents/Frameworks")
    set(RESOURCES_DEST_DIR "${Launcher_Name}.app/Contents/Resources")
    set(JARS_DEST_DIR "${Launcher_Name}.app/Contents/MacOS/jars")

    # Apps to bundle
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${Launcher_Name}.app")

    # Mac bundle settings
    set(MACOSX_BUNDLE_BUNDLE_NAME "${Launcher_Name}")
    set(MACOSX_BUNDLE_INFO_STRING "${Launcher_Name}: A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.prismlauncher.${Launcher_Name}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE ${Launcher_Name}.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2021-2022 ${Launcher_Copyright}")
    set(MACOSX_SPARKLE_UPDATE_PUBLIC_KEY "v55ZWWD6QlPoXGV6VLzOTZxZUggWeE51X8cRQyQh6vA=")
    set(MACOSX_SPARKLE_UPDATE_FEED_URL "https://prismlauncher.org/feed/appcast.xml")

    set(MACOSX_SPARKLE_DOWNLOAD_URL "https://github.com/sparkle-project/Sparkle/releases/download/2.1.0/Sparkle-2.1.0.tar.xz" CACHE STRING "URL to Sparkle release archive")
    set(MACOSX_SPARKLE_SHA256 "bf6ac1caa9f8d321d5784859c88da874f28412f37fb327bc21b7b14c5d61ef94" CACHE STRING "SHA256 checksum for Sparkle release archive")
    set(MACOSX_SPARKLE_DIR "${CMAKE_BINARY_DIR}/frameworks/Sparkle")

    # directories to look for dependencies
    set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${MACOSX_SPARKLE_DIR})

    # install as bundle
    set(INSTALL_BUNDLE "full")

    # Add the icon
    install(FILES ${Launcher_Branding_ICNS} DESTINATION ${RESOURCES_DEST_DIR} RENAME ${Launcher_Name}.icns)

elseif(UNIX)
    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "lib${LIB_SUFFIX}")
    set(JARS_DEST_DIR "share/jars")
    set(LAUNCHER_DESKTOP_DEST_DIR "share/applications" CACHE STRING "Path to the desktop file directory")
    set(LAUNCHER_METAINFO_DEST_DIR "share/metainfo" CACHE STRING "Path to the metainfo directory")
    set(LAUNCHER_ICON_DEST_DIR "share/icons/hicolor/scalable/apps" CACHE STRING "Path to the scalable icon directory")
    set(LAUNCHER_MAN_DEST_DIR "share/man/man6" CACHE STRING "Path to the man page directory")

    # install as bundle with no dependencies included
    set(INSTALL_BUNDLE "nodeps")

    # Set RPATH
    SET(Launcher_BINARY_RPATH "$ORIGIN/")

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_Desktop} DESTINATION ${LAUNCHER_DESKTOP_DEST_DIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_MetaInfo} DESTINATION ${LAUNCHER_METAINFO_DEST_DIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_SVG} DESTINATION ${LAUNCHER_ICON_DEST_DIR})

    if(Launcher_ManPage)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_ManPage} DESTINATION ${LAUNCHER_MAN_DEST_DIR})
    endif()

    # Install basic runner script if component "portable" is selected
    configure_file(launcher/Launcher.in "${CMAKE_CURRENT_BINARY_DIR}/LauncherScript" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/LauncherScript" DESTINATION "." RENAME ${Launcher_Name} COMPONENT portable EXCLUDE_FROM_ALL)

elseif(WIN32)
    set(BINARY_DEST_DIR ".")
    set(LIBRARY_DEST_DIR ".")
    set(PLUGIN_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")
    set(JARS_DEST_DIR "jars")

    # Apps to bundle
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${Launcher_Name}.exe")

    # directories to look for dependencies
    set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    # install as bundle
    set(INSTALL_BUNDLE "full")
else()
    message(FATAL_ERROR "Platform not supported")
endif()

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

option(NBT_BUILD_SHARED "Build NBT shared library" OFF)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
add_subdirectory(libraries/libnbtplusplus)

add_subdirectory(libraries/systeminfo) # system information library
add_subdirectory(libraries/hoedown) # markdown parser
add_subdirectory(libraries/launcher) # java based launcher part for Minecraft
add_subdirectory(libraries/javacheck) # java compatibility checker
if (FORCE_BUNDLED_QUAZIP)
    message(STATUS "Using bundled QuaZip")
    set(BUILD_SHARED_LIBS 0)  # link statically to avoid conflicts.
    set(QUAZIP_INSTALL 0)
    add_subdirectory(libraries/quazip) # zip manipulation library
else()
    message(STATUS "Using system QuaZip")
endif()
add_subdirectory(libraries/rainbow) # Qt extension for colors
add_subdirectory(libraries/LocalPeer) # fork of a library from Qt solutions
if(NOT tomlplusplus_FOUND)
    message(STATUS "Using bundled tomlplusplus")
    add_subdirectory(libraries/tomlplusplus) # toml parser
else()
    message(STATUS "Using system tomlplusplus")
endif()
add_subdirectory(libraries/katabasis) # An OAuth2 library that tried to do too much
add_subdirectory(libraries/gamemode)
add_subdirectory(libraries/murmur2) # Hash for usage with the CurseForge API
if (NOT ghc_filesystem_FOUND)
    message(STATUS "Using bundled ghc_filesystem")
    set(GHC_FILESYSTEM_WITH_INSTALL OFF) # Workaround ghc::filesystem bug
    add_subdirectory(libraries/filesystem) # Implementation of std::filesystem for old C++, for usage in old macOS
    add_library(ghcFilesystem::ghc_filesystem ALIAS ghc_filesystem)
else()
    message(STATUS "Using system ghc_filesystem")
endif()

############################### Built Artifacts ###############################

add_subdirectory(buildconfig)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
# NOTE: this must always be last to appease the CMake deity of quirky install command evaluation order.
add_subdirectory(launcher)
