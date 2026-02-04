cmake_minimum_required(VERSION 3.22)  # minimum version required by Qt

project(Launcher LANGUAGES C CXX)
if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
include(GenerateExportHeader)

add_compile_definitions($<$<CONFIG:Release>:QT_NO_DEBUG>)

if(MSVC)
    # /GS Adds buffer security checks, default on but incuded anyway to mirror gcc's fstack-protector flag
    # /permissive- specify standards-conforming compiler behavior, also enabled by Qt6, default on with std:c++20
    # Use /W4 as /Wall includes unnesserey warnings such as added padding to structs
    set(CMAKE_CXX_FLAGS "/GS /permissive- /W4 ${CMAKE_CXX_FLAGS}")

    # /EHs Enables stack unwind semantics for standard C++ exceptions to ensure stackframes are unwound
    # and object deconstructors are called when an exception is caught.
    # without it memory leaks and a warning is printed
    # /EHc tells the compiler to assume that functions declared as extern "C" never throw a C++ exception
    # This appears to not always be a defualt compiler option in CMAKE
    set(CMAKE_CXX_FLAGS "/EHsc ${CMAKE_CXX_FLAGS}")

    # LINK accepts /SUBSYSTEM whics sets if we are a WINDOWS (gui) or a CONSOLE programs
    # This implicitly selects an entrypoint specific to the subsystem selected
    # qtmain/QtEntryPointLib provides the correct entrypoint (wWinMain) for gui programs
    # Additinaly LINK autodetects we use a GUI so we can omit /SUBSYSTEM
    # This allows tests to still use have console without using seperate linker flags
    # /LTCG allows for linking wholy optimizated programs
    # /MANIFEST:NO disables generating a manifest file, we instead provide our own
    # /STACK sets the stack reserve size, ATL's pack list needs 3-4 MiB as of November 2022, provide 8 MiB
    set(CMAKE_EXE_LINKER_FLAGS "/LTCG /MANIFEST:NO /STACK:8388608 ${CMAKE_EXE_LINKER_FLAGS}")

    # /GL enables whole program optimizations
    # /Gw helps reduce binary size
    # /Gy allows the compiler to package individual functions
    # /guard:cf enables control flow guard
    foreach(lang C CXX)
        set("CMAKE_${lang}_FLAGS_RELEASE" "/GL /Gw /Gy /guard:cf")
    endforeach()

    # See https://github.com/ccache/ccache/issues/1040
    # Note, CMake 3.25 replaces this with CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    # See https://cmake.org/cmake/help/v3.25/variable/CMAKE_MSVC_DEBUG_INFORMATION_FORMAT.html
    foreach(config DEBUG RELWITHDEBINFO)
        foreach(lang C CXX)
            set(flags_var "CMAKE_${lang}_FLAGS_${config}")
            string(REGEX REPLACE "/Z[Ii]" "/Z7" ${flags_var} "${${flags_var}}")
        endforeach()
    endforeach()

    if(CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDLL")
        set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release "")
        set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release "")
    endif()
else()
    set(CMAKE_CXX_FLAGS "-Wall -pedantic -fstack-protector-strong --param=ssp-buffer-size=4 ${CMAKE_CXX_FLAGS}")

    # ATL's pack list needs more than the default 1 Mib stack on windows
    if(WIN32)
        set(CMAKE_EXE_LINKER_FLAGS "-Wl,--stack,8388608 ${CMAKE_EXE_LINKER_FLAGS}")

        # -ffunction-sections and -fdata-sections help reduce binary size
        # -mguard=cf enables Control Flow Guard
        # TODO: Look into -gc-sections to further reduce binary size
        foreach(lang C CXX)
            set("CMAKE_${lang}_FLAGS_RELEASE" "-ffunction-sections -fdata-sections -mguard=cf")
        endforeach()
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_WARN_DEPRECATED_UP_TO=0x060400")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_DISABLE_DEPRECATED_UP_TO=0x060400")

# set CXXFLAGS for build targets
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -D_FORTIFY_SOURCE=2 ${CMAKE_CXX_FLAGS_RELEASE}")

# Export compile commands for debug builds if we can (useful in LSPs like clangd)
# https://cmake.org/cmake/help/v3.31/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles" OR CMAKE_GENERATOR MATCHES "^Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(DEBUG_ADDRESS_SANITIZER "Enable Address Sanitizer in Debug builds" OFF)

# If this is a Debug build turn on address sanitiser
if (DEBUG_ADDRESS_SANITIZER)
    message(STATUS "Address Sanitizer enabled for Debug builds, Turn it off with -DDEBUG_ADDRESS_SANITIZER=off")

    set(USE_ASAN_COMPILE_OPTIONS $<AND:$<CONFIG:Debug,RelWithDebInfo>,$<COMPILE_LANGUAGE:C,CXX>>)
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        message(STATUS "Using Address Sanitizer compile options for MSVC frontend")
        add_compile_options(
                $<${USE_ASAN_COMPILE_OPTIONS}:/fsanitize=address>
                $<${USE_ASAN_COMPILE_OPTIONS}:/Oy->
        )
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        message(STATUS "Using Address Sanitizer compile options for GCC/Clang")
        add_compile_options(
                $<${USE_ASAN_COMPILE_OPTIONS}:-fsanitize=address,undefined>
                $<${USE_ASAN_COMPILE_OPTIONS}:-fno-omit-frame-pointer>
                $<${USE_ASAN_COMPILE_OPTIONS}:-fno-sanitize-recover=null>
        )
        link_libraries("asan" "ubsan")
    else()
        message(STATUS "Address Sanitizer not available on compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()


option(ENABLE_LTO "Enable Link Time Optimization" off)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
        if(CMAKE_BUILD_TYPE)
            if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
                message(STATUS "IPO / LTO enabled")
            else()
                message(STATUS "Not enabling IPO / LTO on debug builds")
            endif()
        else()
            message(STATUS "IPO / LTO will only be enabled for release builds")
        endif()
    else()
        message(STATUS "IPO / LTO not supported: <${ipo_error}>")
    endif()
endif()

option(BUILD_TESTING "Build the testing tree." ON)

find_package(ECM NO_MODULE REQUIRED)
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
set(Launcher_LOGIN_CALLBACK_URL "https://prismlauncher.org/successful-login" CACHE STRING "URL that gets opened when the user successfully logins.")
set(Launcher_LEGACY_FMLLIBS_BASE_URL "https://files.prismlauncher.org/fmllibs/" CACHE STRING "URL for legacy (<=1.5.2) FML Libraries.")

######## Set version numbers ########
set(Launcher_VERSION_MAJOR 11)
set(Launcher_VERSION_MINOR 0)
set(Launcher_VERSION_PATCH 0)

set(Launcher_VERSION_NAME "${Launcher_VERSION_MAJOR}.${Launcher_VERSION_MINOR}.${Launcher_VERSION_PATCH}")
set(Launcher_VERSION_NAME4 "${Launcher_VERSION_MAJOR}.${Launcher_VERSION_MINOR}.${Launcher_VERSION_PATCH}.0")
set(Launcher_VERSION_NAME4_COMMA "${Launcher_VERSION_MAJOR},${Launcher_VERSION_MINOR},${Launcher_VERSION_PATCH},0")

# Build platform.
set(Launcher_BUILD_PLATFORM "unknown" CACHE STRING "A short string identifying the platform that this build was built for. Only used to display in the about dialog.")

# Github repo URL with releases for updater
set(Launcher_UPDATER_GITHUB_REPO "https://github.com/PrismLauncher/PrismLauncher" CACHE STRING "Base github URL for the updater.")

# Name to help updater identify valid artifacts
set(Launcher_BUILD_ARTIFACT "" CACHE STRING "Artifact name to help the updater identify valid artifacts.")

# The metadata server
set(Launcher_META_URL "https://meta.prismlauncher.org/v1/" CACHE STRING "URL to fetch Launcher's meta files from.")

# Imgur API Client ID
set(Launcher_IMGUR_CLIENT_ID "5b97b0713fba4a3" CACHE STRING "Client ID you can get from Imgur when you register an application")

# Bug tracker URL
set(Launcher_BUG_TRACKER_URL "https://github.com/PrismLauncher/PrismLauncher/issues" CACHE STRING "URL for the bug tracker.")

# Translations Platform URL
set(Launcher_TRANSLATIONS_URL "https://hosted.weblate.org/projects/prismlauncher/launcher/" CACHE STRING "URL for the translations platform.")
set(Launcher_TRANSLATION_FILES_URL "https://i18n.prismlauncher.org/" CACHE STRING "URL for the translations files.")

# Matrix Space
set(Launcher_MATRIX_URL "https://prismlauncher.org/matrix" CACHE STRING "URL to the Matrix Space")

# Discord URL
set(Launcher_DISCORD_URL "https://prismlauncher.org/discord" CACHE STRING "URL for the Discord guild.")

# Subreddit URL
set(Launcher_SUBREDDIT_URL "https://prismlauncher.org/reddit" CACHE STRING "URL for the subreddit.")

# Builds
set(Launcher_QT_VERSION_MAJOR "6" CACHE STRING "Major Qt version to build against")

option(Launcher_USE_PCH "Use precompiled headers where possible" ON)

# Java downloader
set(Launcher_ENABLE_JAVA_DOWNLOADER_DEFAULT ON)

# Although we recommend enabling this, we cannot guarantee binary compatibility on
# differing Linux/BSD/etc distributions. Downstream packagers should be explicitly opt-ing into this
# feature if they know it will work with their distribution.
if(UNIX AND NOT APPLE)
    set(Launcher_ENABLE_JAVA_DOWNLOADER_DEFAULT OFF)
endif()

# Java downloader
option(Launcher_ENABLE_JAVA_DOWNLOADER "Build the java downloader feature" ${Launcher_ENABLE_JAVA_DOWNLOADER_DEFAULT})

# Native libraries
if(UNIX AND APPLE)
    set(Launcher_GLFW_LIBRARY_NAME "libglfw.dylib" CACHE STRING "Name of native glfw library")
    set(Launcher_OPENAL_LIBRARY_NAME "libopenal.dylib" CACHE STRING "Name of native openal library")
elseif(UNIX)
    set(Launcher_GLFW_LIBRARY_NAME "libglfw.so" CACHE STRING "Name of native glfw library")
    set(Launcher_OPENAL_LIBRARY_NAME "libopenal.so" CACHE STRING "Name of native openal library")
elseif(WIN32)
    set(Launcher_GLFW_LIBRARY_NAME "glfw.dll" CACHE STRING "Name of native glfw library")
    set(Launcher_OPENAL_LIBRARY_NAME "OpenAL.dll" CACHE STRING "Name of native openal library")
endif()

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

set(Launcher_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
set(Launcher_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
set(Launcher_COMPILER_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
set(Launcher_COMPILER_TARGET_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(Launcher_COMPILER_TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

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
if(Launcher_QT_VERSION_MAJOR EQUAL 6)
    set(QT_VERSION_MAJOR 6)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Core CoreTools Widgets Concurrent Network Test Xml NetworkAuth OpenGL)
    find_package(Qt6 COMPONENTS DBus)
    list(APPEND Launcher_QT_DBUS Qt6::DBus)
else()
    message(FATAL_ERROR "Qt version ${Launcher_QT_VERSION_MAJOR} is not supported")
endif()

if(Launcher_QT_VERSION_MAJOR EQUAL 6)
    set(QT_PLUGINS_DIR  ${QT${QT_VERSION_MAJOR}_INSTALL_PREFIX}/${QT${QT_VERSION_MAJOR}_INSTALL_PLUGINS})
    set(QT_LIBS_DIR     ${QT${QT_VERSION_MAJOR}_INSTALL_PREFIX}/${QT${QT_VERSION_MAJOR}_INSTALL_LIBS})
    set(QT_LIBEXECS_DIR ${QT${QT_VERSION_MAJOR}_INSTALL_PREFIX}/${QT${QT_VERSION_MAJOR}_INSTALL_LIBEXECS})
endif()

find_package(cmark REQUIRED)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(gamemode REQUIRED IMPORTED_TARGET gamemode)
endif()

# Find libqrencode
## NOTE(@getchoo): Never use pkg-config with MSVC since the vcpkg port makes our install bundle fail to find the dll
if(MSVC)
    find_path(LIBQRENCODE_INCLUDE_DIR qrencode.h REQUIRED)
    find_library(LIBQRENCODE_LIBRARY_RELEASE qrencode REQUIRED)
    find_library(LIBQRENCODE_LIBRARY_DEBUG qrencoded)
    set(LIBQRENCODE_LIBRARIES optimized ${LIBQRENCODE_LIBRARY_RELEASE} debug ${LIBQRENCODE_LIBRARY_DEBUG})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libqrencode REQUIRED IMPORTED_TARGET libqrencode)
endif()

# Find libarchive through CMake packages, mainly for vcpkg
find_package(LibArchive)
# CMake packages aren't available in most distributions of libarchive, so fallback to pkg-config
if(NOT LibArchive_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libarchive REQUIRED IMPORTED_TARGET libarchive)
endif()

find_package(tomlplusplus 3.2.0)
# fallback to pkgconfig, important especially as many distros package toml++ built with meson
if(NOT tomlplusplus_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus>=3.2.0)
endif()

find_package(ZLIB REQUIRED)


include(ECMQtDeclareLoggingCategory)

####################################### Program Info #######################################

set(Launcher_APP_BINARY_NAME "prismlauncher" CACHE STRING "Name of the Launcher binary")
add_subdirectory(program_info)

####################################### Install layout #######################################

set(Launcher_ENABLE_UPDATER NO)
set(Launcher_BUILD_UPDATER NO)

if (NOT APPLE AND (NOT Launcher_UPDATER_GITHUB_REPO STREQUAL "" AND NOT Launcher_BUILD_ARTIFACT STREQUAL ""))
    set(Launcher_BUILD_UPDATER YES)
endif()

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

    # Mac bundle settings
    set(MACOSX_BUNDLE_BUNDLE_NAME "${Launcher_DisplayName}")
    set(MACOSX_BUNDLE_INFO_STRING "${Launcher_DisplayName}: A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${Launcher_AppID}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${Launcher_VERSION_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE ${Launcher_Name}.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "${Launcher_Copyright_Mac}")
    set(MACOSX_SPARKLE_UPDATE_PUBLIC_KEY "v55ZWWD6QlPoXGV6VLzOTZxZUggWeE51X8cRQyQh6vA=" CACHE STRING "Public key for Sparkle update feed")
    set(MACOSX_SPARKLE_UPDATE_FEED_URL "https://prismlauncher.org/feed/appcast.xml" CACHE STRING "URL for Sparkle update feed")

    set(MACOSX_SPARKLE_DOWNLOAD_URL "https://github.com/sparkle-project/Sparkle/releases/download/2.8.0/Sparkle-2.8.0.tar.xz" CACHE STRING "URL to Sparkle release archive")
    set(MACOSX_SPARKLE_SHA256 "fd5681ee92bf238aaac2d08214ceaf0cc8976e452d7f882d80bac1e61581f3b1" CACHE STRING "SHA256 checksum for Sparkle release archive")
    set(MACOSX_SPARKLE_DIR "${CMAKE_BINARY_DIR}/frameworks/Sparkle")

    if(NOT MACOSX_SPARKLE_UPDATE_PUBLIC_KEY STREQUAL "" AND NOT MACOSX_SPARKLE_UPDATE_FEED_URL STREQUAL "")
        set(Launcher_ENABLE_UPDATER YES)
    endif()

    # Add the icon
    install(FILES ${Launcher_Branding_ICNS} DESTINATION ${RESOURCES_DEST_DIR} RENAME ${Launcher_Name}.icns)

    find_program(ACTOOL_EXE actool DOC "Path to the apple asset catalog compiler")
    if(ACTOOL_EXE)
        execute_process(
            COMMAND xcodebuild -version
            OUTPUT_VARIABLE XCODE_VERSION_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        string(REGEX MATCH "Xcode ([0-9]+\.[0-9]+)" XCODE_VERSION_MATCH "${XCODE_VERSION_OUTPUT}")
        if(XCODE_VERSION_MATCH)
            set(XCODE_VERSION ${CMAKE_MATCH_1})
        else()
            set(XCODE_VERSION 0.0)
        endif()

        if(XCODE_VERSION VERSION_GREATER_EQUAL 26.0)
            set(ASSETS_OUT_DIR "${CMAKE_BINARY_DIR}/program_info")
            set(GENERATED_ASSETS_CAR "${ASSETS_OUT_DIR}/Assets.car")
            set(ICON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_Branding_MAC_ICON}")

            add_custom_command(
                OUTPUT "${GENERATED_ASSETS_CAR}"
                COMMAND ${ACTOOL_EXE} "${ICON_SOURCE}"
                        --compile "${ASSETS_OUT_DIR}"
                        --output-partial-info-plist /dev/null
                        --app-icon PrismLauncher
                        --enable-on-demand-resources NO
                        --target-device mac
                        --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET}
                        --platform macosx
                DEPENDS "${ICON_SOURCE}"
                COMMENT "Compiling asset catalog (${ICON_SOURCE})"
                VERBATIM
            )
            add_custom_target(compile_assets ALL DEPENDS "${GENERATED_ASSETS_CAR}")
            install(FILES "${GENERATED_ASSETS_CAR}" DESTINATION "${RESOURCES_DEST_DIR}")
        else()
            message(WARNING "Xcode ${XCODE_VERSION} is too old. Minimum required version is 26.0. Not compiling liquid glass icons.")
        endif()

    else()
        message(WARNING "actool not found. Cannot compile macOS app icons.\n"
                "Install Xcode command line tools: 'xcode-select --install'")
    endif()


elseif(UNIX)
    include(KDEInstallDirs)

    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "lib${LIB_SUFFIX}")
    set(JARS_DEST_DIR "share/${Launcher_Name}")

    # Set RPATH
    SET(Launcher_BINARY_RPATH "$ORIGIN/")

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_Desktop} DESTINATION ${KDE_INSTALL_APPDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_MetaInfo} DESTINATION ${KDE_INSTALL_METAINFODIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_SVG} DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_PNG_256} DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/256x256/apps" RENAME "${Launcher_AppID}.png")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${Launcher_mrpack_MIMEInfo} DESTINATION ${KDE_INSTALL_MIMEDIR})

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/launcher/qtlogging.ini" DESTINATION "share/${Launcher_Name}")

    set(PLUGIN_DEST_DIR "plugins")
    set(BUNDLE_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")

    if(Launcher_ManPage)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${Launcher_ManPage} DESTINATION "${KDE_INSTALL_MANDIR}/man6")
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

add_subdirectory(libraries/launcher) # java based launcher part for Minecraft
add_subdirectory(libraries/javacheck) # java compatibility checker

add_subdirectory(libraries/rainbow) # Qt extension for colors
add_subdirectory(libraries/LocalPeer) # fork of a library from Qt solutions
add_subdirectory(libraries/murmur2) # Hash for usage with the CurseForge API
add_subdirectory(libraries/qdcss) # css parser

############################### Built Artifacts ###############################

add_subdirectory(buildconfig)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
# NOTE: this must always be last to appease the CMake deity of quirky install command evaluation order.
add_subdirectory(launcher)
