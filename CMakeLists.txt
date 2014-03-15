cmake_minimum_required(VERSION 2.8.9)

IF(WIN32)
      # In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
      cmake_policy(SET CMP0020 OLD)
ENDIF()

project(MultiMC)
enable_testing()

######## Set CMake options ########
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(FILES_TO_TRANSLATE )

######## Set module path ########
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")
SET(MMC_SRC "${PROJECT_SOURCE_DIR}")
SET(MMC_BIN "${PROJECT_BINARY_DIR}")

# Output all executables and shared libs in the main build folder, not in subfolders.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

IF(UNIX)
      SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
ENDIF()

set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
IF(APPLE)
      message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
ELSEIF(UNIX)
      # assume GCC, add C++0x/C++11 stuff
      MESSAGE(STATUS "Using UNIX CMAKE_CXX_FLAGS")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
ELSEIF(MINGW)
      MESSAGE(STATUS "Using MINGW CMAKE_CXX_FLAGS")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
ENDIF()

################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

include_directories(
      ${Qt5Core_INCLUDE_DIRS}
      ${Qt5Widgets_INCLUDE_DIRS}
      ${Qt5Concurrent_INCLUDE_DIRS}
      ${Qt5Network_INCLUDE_DIRS}
      ${Qt5Test_INCLUDE_DIRS}
  )

# The Qt5 cmake files don't provide its install paths, so ask qmake.
get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)
function(QUERY_QMAKE VAR RESULT)
      exec_program(${QMAKE_EXECUTABLE} ARGS "-query ${VAR}" RETURN_VALUE return_code OUTPUT_VARIABLE output )
      if(NOT return_code)
            file(TO_CMAKE_PATH "${output}" output)
            set(${RESULT} ${output} PARENT_SCOPE)
      endif(NOT return_code)
endfunction(QUERY_QMAKE)

query_qmake(QT_INSTALL_PLUGINS QT_PLUGINS_DIR)
query_qmake(QT_INSTALL_IMPORTS QT_IMPORTS_DIR)
query_qmake(QT_INSTALL_LIBS QT_LIBS_DIR)
query_qmake(QT_HOST_DATA QT_DATA_DIR)
set(QT_MKSPECS_DIR ${QT_DATA_DIR}/mkspecs)


################################ SET UP BUILD OPTIONS ################################

######## Check endianness ########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)
IF(${BIGENDIAN})
      ADD_DEFINITIONS(-DMULTIMC_BIG_ENDIAN)
ENDIF(${BIGENDIAN})


######## Set URLs ########

SET(MultiMC_NEWS_RSS_URL "http://multimc.org/rss.xml" CACHE STRING "URL to fetch MultiMC's news RSS feed from.")


######## Set version numbers ########
SET(MultiMC_VERSION_MAJOR		0)
SET(MultiMC_VERSION_MINOR		2)
SET(MultiMC_VERSION_HOTFIX		1)

# Build number
SET(MultiMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Version type
SET(MultiMC_VERSION_TYPE "Custom" CACHE STRING "MultiMC's version type. This should be one of 'Custom', 'Release', 'ReleaseCandidate', or 'Development', depending on what type of version this is.")

# Build platform.
SET(MultiMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Version channel
SET(MultiMC_VERSION_CHANNEL "" CACHE STRING "The current build's channel. Included in the version string.")

# Channel list URL
SET(MultiMC_CHANLIST_URL "" CACHE STRING "URL for the channel list.")

# Updater enabled?
SET(MultiMC_UPDATER false CACHE BOOL "Whether or not the update system is enabled. If this is enabled, you must also set MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL in order for it to work properly.")

# Notification URL
SET(MultiMC_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

SET(MultiMC_RELEASE_VERSION_NAME "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}")
IF (MultiMC_VERSION_HOTFIX GREATER 0)
      SET(MultiMC_RELEASE_VERSION_NAME "${MultiMC_RELEASE_VERSION_NAME}.${MultiMC_VERSION_HOTFIX}")
ENDIF()

# Build a version string to display in the configure logs.
IF (MultiMC_VERSION_TYPE STREQUAL "Custom")
      MESSAGE(STATUS "Version Type: Custom")
      SET(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
ELSEIF (MultiMC_VERSION_TYPE STREQUAL "Release")
      MESSAGE(STATUS "Version Type: Stable Release")
      SET(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
ELSEIF (MultiMC_VERSION_TYPE STREQUAL "ReleaseCandidate")
      MESSAGE(STATUS "Version Type: Release Candidate")
      SET(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}-rc${MultiMC_VERSION_BUILD}")
ELSEIF (MultiMC_VERSION_TYPE STREQUAL "Development")
      MESSAGE(STATUS "Version Type: Development")
      SET(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}-dev${MultiMC_VERSION_BUILD}")
ELSE ()
      MESSAGE(ERROR "Invalid build type.")
ENDIF ()

MESSAGE(STATUS "MultiMC 5 Version: ${MultiMC_VERSION_STRING}")

# If the update system is enabled, make sure MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL are set.
IF (MultiMC_UPDATER)
      IF (MultiMC_VERSION_CHANNEL STREQUAL "")
            MESSAGE(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_CHANNEL is not set.\n"
                                          "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
      ENDIF ()
      IF (MultiMC_CHANLIST_URL STREQUAL "")
            MESSAGE(FATAL_ERROR "Update system is enabled, but MultiMC_CHANLIST_URL is not set.\n"
                                          "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
      ENDIF ()
      IF (MultiMC_VERSION_BUILD LESS 0)
            MESSAGE(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_BUILD is not set.\n"
                                          "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
      ENDIF ()

      MESSAGE(STATUS "Updater is enabled. Channel list URL: ${MultiMC_CHANLIST_URL}")
ENDIF ()

#### Custom target to just print the version.
ADD_CUSTOM_TARGET(version echo "Version: ${MultiMC_VERSION_STRING}")

#### Check the current Git commit
execute_process(COMMAND git rev-parse HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE GIT_COMMIT_CHECK_RESULTVAR
      OUTPUT_VARIABLE GIT_COMMIT_CHECK_OUTVAR
      OUTPUT_STRIP_TRAILING_WHITESPACE
)

IF(GIT_COMMIT_CHECK_RESULTVAR EQUAL 0)
      SET(MultiMC_GIT_COMMIT "${GIT_COMMIT_CHECK_OUTVAR}")
      MESSAGE(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")
ELSE()
      SET(MultiMC_GIT_COMMIT "Unknown")
      MESSAGE(STATUS "Failed to check Git commit. ${GIT_COMMIT_CHECK_RESULTVAR}")
ENDIF()

######## Configure header ########
configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/include/config.h")


######## Other Stuff ########

ADD_DEFINITIONS(-DQUAZIP_STATIC)
ADD_DEFINITIONS(-DLIBSETTINGS_STATIC)
ADD_DEFINITIONS(-DLIBUTIL_STATIC)

######## Packaging/install paths setup ########

IF(UNIX AND APPLE)
      SET(BINARY_DEST_DIR MultiMC.app/Contents/MacOS)
      SET(PLUGIN_DEST_DIR MultiMC.app/Contents/MacOS)
      SET(QTCONF_DEST_DIR MultiMC.app/Contents/Resources)
      SET(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")

      SET(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
      SET(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
      SET(MACOSX_BUNDLE_GUI_IDENTIFIER "org.multimc.MultiMC5")
      SET(MACOSX_BUNDLE_BUNDLE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      SET(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
      SET(MACOSX_BUNDLE_COPYRIGHT "Copyright 2013 MultiMC Contributors")
ELSEIF(UNIX)
      SET(BINARY_DEST_DIR bin)
      SET(PLUGIN_DEST_DIR plugins)
      SET(QTCONF_DEST_DIR .)
      SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")
ELSEIF(WIN32)
      SET(BINARY_DEST_DIR .)
      SET(PLUGIN_DEST_DIR .)
      SET(QTCONF_DEST_DIR .)
      SET(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.exe")
ENDIF()

# directories to look for dependencies
SET(DIRS "${QT_LIBS_DIR}")

################################ Included Libs ################################

# Add quazip
add_subdirectory(depends/quazip)
include_directories(depends/quazip)

# Add the java launcher and checker
add_subdirectory(depends/launcher)
add_subdirectory(depends/javacheck)

# Add xz decompression
add_subdirectory(depends/xz-embedded)
include_directories(${XZ_INCLUDE_DIR})

# Add pack200 decompression
add_subdirectory(depends/pack200)
include_directories(${PACK200_INCLUDE_DIR})

######## MultiMC Libs ########

# Add the util library.
add_subdirectory(depends/util)
include_directories(${LIBUTIL_INCLUDE_DIR})

# Add the settings library.
add_subdirectory(depends/settings)
include_directories(${LIBSETTINGS_INCLUDE_DIR})

# Add the updater
add_subdirectory(mmc_updater)

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
# Application base
MultiMC.h
MultiMC.cpp
MultiMCVersion.h
MMCError.h

# Logging
logger/QsDebugOutput.cpp
logger/QsDebugOutput.h
logger/QsLog.cpp
logger/QsLog.h
logger/QsLogDest.cpp
logger/QsLogDest.h

# GUI - windows
gui/MainWindow.h
gui/MainWindow.cpp
gui/ConsoleWindow.h
gui/ConsoleWindow.cpp

# GUI - dialogs
gui/dialogs/SettingsDialog.h
gui/dialogs/SettingsDialog.cpp
gui/dialogs/CopyInstanceDialog.h
gui/dialogs/CopyInstanceDialog.cpp
gui/dialogs/NewInstanceDialog.cpp
gui/dialogs/ProgressDialog.h
gui/dialogs/ProgressDialog.cpp
gui/dialogs/AboutDialog.h
gui/dialogs/AboutDialog.cpp
gui/dialogs/VersionSelectDialog.h
gui/dialogs/VersionSelectDialog.cpp
gui/dialogs/LwjglSelectDialog.h
gui/dialogs/LwjglSelectDialog.cpp
gui/dialogs/InstanceSettings.h
gui/dialogs/InstanceSettings.cpp
gui/dialogs/IconPickerDialog.h
gui/dialogs/IconPickerDialog.cpp
gui/dialogs/LegacyModEditDialog.h
gui/dialogs/LegacyModEditDialog.cpp
gui/dialogs/OneSixModEditDialog.h
gui/dialogs/OneSixModEditDialog.cpp
gui/dialogs/ModEditDialogCommon.h
gui/dialogs/ModEditDialogCommon.cpp
gui/dialogs/EditNotesDialog.h
gui/dialogs/EditNotesDialog.cpp
gui/dialogs/CustomMessageBox.h
gui/dialogs/CustomMessageBox.cpp
gui/dialogs/EditAccountDialog.h
gui/dialogs/EditAccountDialog.cpp
gui/dialogs/AccountListDialog.h
gui/dialogs/AccountListDialog.cpp
gui/dialogs/AccountSelectDialog.h
gui/dialogs/AccountSelectDialog.cpp
gui/dialogs/UpdateDialog.h
gui/dialogs/UpdateDialog.cpp
gui/dialogs/ScreenshotDialog.h
gui/dialogs/ScreenshotDialog.cpp
gui/dialogs/NotificationDialog.h
gui/dialogs/NotificationDialog.cpp

# GUI - widgets
gui/widgets/Common.h
gui/widgets/Common.cpp
gui/widgets/ModListView.h
gui/widgets/ModListView.cpp
gui/widgets/VersionListView.h
gui/widgets/VersionListView.cpp
gui/widgets/LabeledToolButton.h
gui/widgets/LabeledToolButton.cpp
gui/widgets/MCModInfoFrame.h
gui/widgets/MCModInfoFrame.cpp

# GUI - instance group view
gui/groupview/Group.cpp
gui/groupview/Group.h
gui/groupview/GroupedProxyModel.cpp
gui/groupview/GroupedProxyModel.h
gui/groupview/GroupView.cpp
gui/groupview/GroupView.h
gui/groupview/InstanceDelegate.cpp
gui/groupview/InstanceDelegate.h

# Base classes and infrastructure
logic/BaseVersion.h
logic/MinecraftVersion.h
logic/InstanceFactory.h
logic/InstanceFactory.cpp
logic/BaseInstance.h
logic/BaseInstance.cpp
logic/BaseInstance_p.h

logic/MinecraftProcess.h
logic/MinecraftProcess.cpp
logic/Mod.h
logic/Mod.cpp
logic/ModList.h
logic/ModList.cpp

# Basic instance launcher for starting from terminal
logic/InstanceLauncher.h
logic/InstanceLauncher.cpp

# JSON parsing helpers
logic/MMCJson.h
logic/MMCJson.cpp

# network stuffs
logic/net/NetAction.h
logic/net/MD5EtagDownload.h
logic/net/MD5EtagDownload.cpp
logic/net/ByteArrayDownload.h
logic/net/ByteArrayDownload.cpp
logic/net/CacheDownload.h
logic/net/CacheDownload.cpp
logic/net/ForgeMirrors.h
logic/net/ForgeMirrors.cpp
logic/net/ForgeXzDownload.h
logic/net/ForgeXzDownload.cpp
logic/net/NetJob.h
logic/net/NetJob.cpp
logic/net/HttpMetaCache.h
logic/net/HttpMetaCache.cpp
logic/net/PasteUpload.h
logic/net/PasteUpload.cpp
logic/net/URLConstants.h
logic/net/URLConstants.cpp

# Yggdrasil login stuff
logic/auth/AuthSession.h
logic/auth/AuthSession.cpp
logic/auth/MojangAccountList.h
logic/auth/MojangAccountList.cpp
logic/auth/MojangAccount.h
logic/auth/MojangAccount.cpp
logic/auth/YggdrasilTask.h
logic/auth/YggdrasilTask.cpp
logic/auth/flows/AuthenticateTask.h
logic/auth/flows/AuthenticateTask.cpp
logic/auth/flows/RefreshTask.cpp
logic/auth/flows/RefreshTask.cpp
logic/auth/flows/ValidateTask.h
logic/auth/flows/ValidateTask.cpp

# Update system
logic/updater/UpdateChecker.h
logic/updater/UpdateChecker.cpp
logic/updater/DownloadUpdateTask.h
logic/updater/DownloadUpdateTask.cpp
logic/updater/NotificationChecker.h
logic/updater/NotificationChecker.cpp

# News System
logic/news/NewsChecker.h
logic/news/NewsChecker.cpp
logic/news/NewsEntry.h
logic/news/NewsEntry.cpp

# Status system
logic/status/StatusChecker.h
logic/status/StatusChecker.cpp

# legacy instances
logic/LegacyInstance.h
logic/LegacyInstance.cpp
logic/LegacyInstance_p.h
logic/LegacyUpdate.h
logic/LegacyUpdate.cpp

logic/LegacyForge.h
logic/LegacyForge.cpp

# OneSix instances
logic/OneSixUpdate.h
logic/OneSixUpdate.cpp
logic/OneSixInstance.h
logic/OneSixInstance.cpp
logic/OneSixInstance_p.h

# OneSix version json infrastructure
logic/OneSixVersionBuilder.h
logic/OneSixVersionBuilder.cpp
logic/VersionFile.h
logic/VersionFile.cpp
logic/VersionFinal.h
logic/VersionFinal.cpp
logic/OneSixLibrary.h
logic/OneSixLibrary.cpp
logic/OneSixRule.h
logic/OneSixRule.cpp
logic/OpSys.h
logic/OpSys.cpp

# Mod installers
logic/BaseInstaller.h
logic/BaseInstaller.cpp
logic/ForgeInstaller.h
logic/ForgeInstaller.cpp
logic/LiteLoaderInstaller.h
logic/LiteLoaderInstaller.cpp

# Nostalgia
logic/NostalgiaInstance.h
logic/NostalgiaInstance.cpp

# FTB
logic/OneSixFTBInstance.h
logic/OneSixFTBInstance.cpp
logic/LegacyFTBInstance.h
logic/LegacyFTBInstance.cpp

# Lists
logic/lists/InstanceList.h
logic/lists/InstanceList.cpp
logic/lists/BaseVersionList.h
logic/lists/BaseVersionList.cpp
logic/lists/MinecraftVersionList.h
logic/lists/MinecraftVersionList.cpp
logic/lists/LwjglVersionList.h
logic/lists/LwjglVersionList.cpp
logic/lists/ForgeVersionList.h
logic/lists/ForgeVersionList.cpp
logic/lists/JavaVersionList.h
logic/lists/JavaVersionList.cpp
logic/lists/LiteLoaderVersionList.h
logic/lists/LiteLoaderVersionList.cpp

# the screenshots feature
logic/screenshots/Screenshot.h
logic/screenshots/Screenshot.cpp
logic/screenshots/ScreenshotList.h
logic/screenshots/ScreenshotList.cpp
logic/screenshots/ImgurUpload.h
logic/screenshots/ImgurUpload.cpp
logic/screenshots/ImgurAlbumCreation.h
logic/screenshots/ImgurAlbumCreation.cpp

# Icons
logic/icons/MMCIcon.h
logic/icons/MMCIcon.cpp
logic/icons/IconList.h
logic/icons/IconList.cpp


# misc model/view
logic/EnabledItemFilter.h
logic/EnabledItemFilter.cpp

# Tasks
logic/tasks/ProgressProvider.h
logic/tasks/Task.h
logic/tasks/Task.cpp
logic/tasks/ThreadTask.h
logic/tasks/ThreadTask.cpp
logic/tasks/SequentialTask.h
logic/tasks/SequentialTask.cpp

# Utilities
logic/JavaChecker.h
logic/JavaChecker.cpp
logic/JavaUtils.h
logic/JavaUtils.cpp
logic/NagUtils.h
logic/NagUtils.cpp
logic/SkinUtils.h
logic/SkinUtils.cpp
logic/JavaCheckerJob.h
logic/JavaCheckerJob.cpp

# Assets
logic/assets/AssetsMigrateTask.h
logic/assets/AssetsMigrateTask.cpp
logic/assets/AssetsUtils.h
logic/assets/AssetsUtils.cpp

# Tools
logic/tools/BaseExternalTool.h
logic/tools/BaseExternalTool.cpp
logic/tools/MCEditTool.h
logic/tools/MCEditTool.cpp
logic/tools/BaseProfiler.h
logic/tools/BaseProfiler.cpp
logic/tools/JProfiler.h
logic/tools/JProfiler.cpp
logic/tools/JVisualVM.h
logic/tools/JVisualVM.cpp
)


######## UIs ########
SET(MULTIMC_UIS

# Windows
gui/MainWindow.ui
gui/ConsoleWindow.ui

# Dialogs
gui/dialogs/SettingsDialog.ui
gui/dialogs/CopyInstanceDialog.ui
gui/dialogs/NewInstanceDialog.ui
gui/dialogs/AboutDialog.ui
gui/dialogs/VersionSelectDialog.ui
gui/dialogs/LwjglSelectDialog.ui
gui/dialogs/InstanceSettings.ui
gui/dialogs/ProgressDialog.ui
gui/dialogs/IconPickerDialog.ui
gui/dialogs/LegacyModEditDialog.ui
gui/dialogs/OneSixModEditDialog.ui
gui/dialogs/EditNotesDialog.ui
gui/dialogs/AccountListDialog.ui
gui/dialogs/AccountSelectDialog.ui
gui/dialogs/EditAccountDialog.ui
gui/dialogs/UpdateDialog.ui
gui/dialogs/ScreenshotDialog.ui
gui/dialogs/NotificationDialog.ui

# Widgets/other
gui/widgets/MCModInfoFrame.ui
)

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${MULTIMC_SOURCES} ${MULTIMC_UIS})

SET(MULTIMC_QRCS
resources/backgrounds/backgrounds.qrc
resources/multimc/multimc.qrc
resources/instances/instances.qrc
)


######## Windows resource files ########
IF(WIN32)
SET(MULTIMC_RCS resources/multimc.rc)
ENDIF()

####### X11 Stuff #######
IF(UNIX AND NOT APPLE)
      SET(MultiMC_QT_ADDITIONAL_MODULES ${MultiMC_QT_ADDITIONAL_MODULES} X11Extras)
      SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} xcb)
      LIST(APPEND MULTIMC_SOURCES gui/Platform_X11.cpp)
ELSE()
      LIST(APPEND MULTIMC_SOURCES gui/Platform_Other.cpp)
ENDIF()


################################ COMPILE ################################

# Link additional libraries
IF(WIN32)
      SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
            Qt5::WinMain # Link WinMain
      )
ENDIF(WIN32)

OPTION(MultiMC_UPDATER_DRY_RUN "Enable updater dry-run mode -- for updater development." OFF)
OPTION(MultiMC_UPDATER_FORCE_LOCAL "Do not download updated updater -- for updater development." OFF)

OPTION(MultiMC_CODE_COVERAGE "Compiles for code coverage" OFF)
IF(MultiMC_CODE_COVERAGE)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
      SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
      SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O0 --coverage")
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 --coverage")
ENDIF(MultiMC_CODE_COVERAGE)

# Tell CMake that MultiMCLauncher.jar is generated.
#SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/depends/launcher/MultiMCLauncher.jar GENERATED)
#SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/depends/javacheck/JavaCheck.jar GENERATED)

# Qt 5 stuff
QT5_WRAP_UI(MULTIMC_UI ${MULTIMC_UIS})
QT5_ADD_RESOURCES(MULTIMC_RESOURCES ${MULTIMC_QRCS})

# Add common library
ADD_LIBRARY(MultiMC_common STATIC ${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_RESOURCES})

# Add executable
ADD_EXECUTABLE(MultiMC MACOSX_BUNDLE WIN32 main.cpp ${MULTIMC_RCS})

# Link
TARGET_LINK_LIBRARIES(MultiMC MultiMC_common)
TARGET_LINK_LIBRARIES(MultiMC_common xz-embedded unpack200 quazip libUtil libSettings ${MultiMC_LINK_ADDITIONAL_LIBS})
QT5_USE_MODULES(MultiMC Core Widgets Network Xml Concurrent ${MultiMC_QT_ADDITIONAL_MODULES})
QT5_USE_MODULES(MultiMC_common Core Widgets Network Xml Concurrent ${MultiMC_QT_ADDITIONAL_MODULES})

################################ INSTALLATION AND PACKAGING ################################

######## Install ########

#### Executable ####
IF(APPLE AND UNIX) ## OSX
      INSTALL(TARGETS MultiMC
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION MultiMC.app/Contents/MacOS COMPONENT Runtime
      )

ELSEIF(UNIX) ## LINUX and similar
      INSTALL(TARGETS MultiMC
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION bin COMPONENT Runtime
      )
      INSTALL(PROGRAMS package/linux/MultiMC DESTINATION .)

ELSEIF(WIN32) ## WINDOWS
      INSTALL(TARGETS MultiMC
            BUNDLE DESTINATION . COMPONENT Runtime
            LIBRARY DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION . COMPONENT Runtime
      )
ENDIF()

#### Dist package logic ####

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
# Image formats
INSTALL(
      DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
      DESTINATION ${PLUGIN_DEST_DIR}
      COMPONENT Runtime
      REGEX "tga|svg|tiff|mng" EXCLUDE
)

# Platform plugins
INSTALL(
      DIRECTORY "${QT_PLUGINS_DIR}/platforms"
      DESTINATION ${PLUGIN_DEST_DIR}
      COMPONENT Runtime
      REGEX "minimal|linuxfb|offscreen" EXCLUDE
)
else()
# Image formats
INSTALL(
      DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
      DESTINATION ${PLUGIN_DEST_DIR}
      COMPONENT Runtime
      REGEX "tga|svg|tiff|mng" EXCLUDE
      REGEX "d\\." EXCLUDE
      REGEX "_debug\\." EXCLUDE
)

# Platform plugins
INSTALL(
      DIRECTORY "${QT_PLUGINS_DIR}/platforms"
      DESTINATION ${PLUGIN_DEST_DIR}
      COMPONENT Runtime
      REGEX "minimal|linuxfb|offscreen" EXCLUDE
      REGEX "d\\." EXCLUDE
      REGEX "_debug\\." EXCLUDE
)
IF(APPLE)
      # Accessible plugin to make buttons look decent on osx
      INSTALL(
            DIRECTORY "${QT_PLUGINS_DIR}/accessible"
            DESTINATION ${PLUGIN_DEST_DIR}
            COMPONENT Runtime
            REGEX "quick" EXCLUDE
            REGEX "d\\." EXCLUDE
            REGEX "_debug\\." EXCLUDE
      )
ENDIF()

endif()

# qtconf
INSTALL(
      CODE "
FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
"
      COMPONENT Runtime
)

# ICNS file for OS X
IF(APPLE)
      INSTALL(FILES resources/MultiMC.icns DESTINATION MultiMC.app/Contents/Resources)
ENDIF()

CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/install_prereqs.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake"
      @ONLY)
INSTALL(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake" COMPONENT Runtime)



######## Package ########

# Package with CPack
IF(UNIX)
      if(APPLE)
            SET(CPACK_GENERATOR "ZIP")
      else()
            SET(CPACK_GENERATOR "TGZ")
      endif()
ELSEIF(WIN32)
      SET(CPACK_GENERATOR "ZIP")
ENDIF()
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

SET(CPACK_PACKAGE_NAME "MultiMC 5")
SET(CPACK_PACKAGE_VENDOR "")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MultiMC - Minecraft launcher and management tool.")
SET(CPACK_PACKAGE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${MultiMC_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${MultiMC_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${MultiMC_VERSION_REV})

IF(CPACK_GENERATOR STREQUAL "NSIS")
SET(CPACK_PACKAGE_FILE_NAME "Setup-MultiMC")
ELSE()
SET(CPACK_PACKAGE_FILE_NAME "MultiMC")
ENDIF()

IF(WIN32)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "MultiMC 5")
ENDIF()

INCLUDE(CPack)

include_directories(${PROJECT_BINARY_DIR}/include)

# Translations
add_subdirectory(translations)

# Tests
add_subdirectory(tests)
