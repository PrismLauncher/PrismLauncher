cmake_minimum_required(VERSION 2.8.9)

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
set(FILES_TO_TRANSLATE )

######## Set module path ########
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")
set(MMC_SRC "${PROJECT_SOURCE_DIR}")
set(MMC_BIN "${PROJECT_BINARY_DIR}")

# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if(UNIX)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
include(UseCXX11)
include(Coverage)
set(CMAKE_CXX_FLAGS " -Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")

################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

include_directories(
	${Qt5Core_INCLUDE_DIRS}
	${Qt5Widgets_INCLUDE_DIRS}
	${Qt5Concurrent_INCLUDE_DIRS}
	${Qt5Test_INCLUDE_DIRS}
	${Qt5Network_INCLUDE_DIRS}
	${Qt5Xml_INCLUDE_DIRS}
)

# The Qt5 cmake files don't provide its install paths, so ask qmake.
include(QMakeQuery)
query_qmake(QT_INSTALL_PLUGINS QT_PLUGINS_DIR)
query_qmake(QT_INSTALL_IMPORTS QT_IMPORTS_DIR)
query_qmake(QT_INSTALL_LIBS QT_LIBS_DIR)
query_qmake(QT_HOST_DATA QT_DATA_DIR)
set(QT_MKSPECS_DIR ${QT_DATA_DIR}/mkspecs)


################################ SET UP BUILD OPTIONS ################################

######## Check endianness ########
include(TestBigEndian)
test_big_endian(BIGENDIAN)
if(${BIGENDIAN})
	add_definitions(-DMULTIMC_BIG_ENDIAN)
endif(${BIGENDIAN})

######## Set URLs ########
set(MultiMC_NEWS_RSS_URL "http://multimc.org/rss.xml" CACHE STRING "URL to fetch MultiMC's news RSS feed from.")

######## Set version numbers ########
set(MultiMC_VERSION_MAJOR	 0)
set(MultiMC_VERSION_MINOR	 4)
set(MultiMC_VERSION_HOTFIX   5)

# Build number
set(MultiMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Version type
set(MultiMC_VERSION_TYPE "Custom" CACHE STRING "MultiMC's version type. This should be one of 'Custom', 'Release', 'ReleaseCandidate', or 'Development', depending on what type of version this is.")

# Build platform.
set(MultiMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Version channel
set(MultiMC_VERSION_CHANNEL "" CACHE STRING "The current build's channel. Included in the version string.")

# Channel list URL
set(MultiMC_CHANLIST_URL "" CACHE STRING "URL for the channel list.")

# Updater enabled?
set(MultiMC_UPDATER false CACHE BOOL "Whether or not the update system is enabled. If this is enabled, you must also set MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL in order for it to work properly.")

# Notification URL
set(MultiMC_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}")
if(MultiMC_VERSION_HOTFIX GREATER 0)
	set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_RELEASE_VERSION_NAME}.${MultiMC_VERSION_HOTFIX}")
endif()

# Build a version string to display in the configure logs.
if(MultiMC_VERSION_TYPE STREQUAL "Custom")
	message(STATUS "Version Type: Custom")
	set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
elseif(MultiMC_VERSION_TYPE STREQUAL "Release")
	message(STATUS "Version Type: Stable Release")
	set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
elseif(MultiMC_VERSION_TYPE STREQUAL "Development")
	message(STATUS "Version Type: Development")
	set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}-dev${MultiMC_VERSION_BUILD}")
else()
	message(ERROR "Invalid build type.")
endif()

message(STATUS "MultiMC 5 Version: ${MultiMC_VERSION_STRING}")

# If the update system is enabled, make sure MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL are set.
if(MultiMC_UPDATER)
	if(MultiMC_VERSION_CHANNEL STREQUAL "")
		message(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_CHANNEL is not set.\n"
				"Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
	endif()
	if(MultiMC_CHANLIST_URL STREQUAL "")
	message(FATAL_ERROR "Update system is enabled, but MultiMC_CHANLIST_URL is not set.\n"
				"Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
	endif()
	if(MultiMC_VERSION_BUILD LESS 0)
	message(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_BUILD is not set.\n"
				"Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
	endif()
	message(STATUS "Updater is enabled. Channel list URL: ${MultiMC_CHANLIST_URL}")
endif()

#### Updater-related build config options ####
option(MultiMC_UPDATER_DRY_RUN "Enable updater dry-run mode -- for updater development." OFF)
option(MultiMC_UPDATER_FORCE_LOCAL "Do not download updated updater -- for updater development." OFF)

if(MultiMC_UPDATER_DRY_RUN)
	set(MultiMC_UPDATER_DRY_RUN_value "true")
else()
	set(MultiMC_UPDATER_DRY_RUN_value "false")
endif()

if(MultiMC_UPDATER_FORCE_LOCAL)
	set(MultiMC_UPDATER_FORCE_LOCAL_value "true")
else()
	set(MultiMC_UPDATER_FORCE_LOCAL_value "false")
endif()

#### For QuickMods
option(MultiMC_WEBKIT_INSPECTOR "Enable the QWebInspector for debugging" OFF)
if(MultiMC_WEBKIT_INSPECTOR)
    add_definitions(-DWEBKIT_INSPECTOR)
endif()

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MultiMC_VERSION_STRING}")

#### Check the current Git commit
include(GitFunctions)
git_run(COMMAND rev-parse HEAD DEFAULT "Unknown" OUTPUT_VAR MultiMC_GIT_COMMIT)
message(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")

######## Configure header ########
configure_file("${PROJECT_SOURCE_DIR}/BuildConfig.cpp.in" "${PROJECT_BINARY_DIR}/BuildConfig.cpp")

######## Packaging/install paths setup ########

if(UNIX AND APPLE)
	set(BINARY_DEST_DIR MultiMC.app/Contents/MacOS)
	set(PLUGIN_DEST_DIR MultiMC.app/Contents/MacOS)
	set(QTCONF_DEST_DIR MultiMC.app/Contents/Resources)
	set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")

	set(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
	set(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.multimc.MultiMC5")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}.${MultiMC_VERSION_BUILD}")
	set(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
	set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2014 MultiMC Contributors")
elseif(UNIX)
	set(BINARY_DEST_DIR bin)
	set(PLUGIN_DEST_DIR plugins)
	set(QTCONF_DEST_DIR .)
	set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")
elseif(WIN32)
	set(BINARY_DEST_DIR .)
	set(PLUGIN_DEST_DIR .)
	set(QTCONF_DEST_DIR .)
	set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.exe")
endif()

# directories to look for dependencies
set(DIRS "${QT_LIBS_DIR}")

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

# Add quazip
add_definitions(-DQUAZIP_STATIC)
set(QUAZIP_VERSION "0.7.1")
if(NOT EXISTS ${CMAKE_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz)
	file(DOWNLOAD http://downloads.sourceforge.net/project/quazip/quazip/${QUAZIP_VERSION}/quazip-${QUAZIP_VERSION}.tar.gz ${CMAKE_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz)
endif()
ExternalProject_Add(QuaZIP
	SOURCE_DIR <BINARY_DIR>/../Source/quazip-${QUAZIP_VERSION}
	DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>/.. ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/quazip-${QUAZIP_VERSION}.tar.gz
	PATCH_COMMAND patch -p0 -i ${CMAKE_SOURCE_DIR}/quazip.patch
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
include_directories("${CMAKE_BINARY_DIR}/External/Install/QuaZIP/include/quazip")
set(QUAZIP_LIBRARIES -L"${CMAKE_BINARY_DIR}/External/Install/QuaZIP/lib" quazip)

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
add_definitions(-DLIBUTIL_STATIC)
add_subdirectory(depends/util)
include_directories(${LIBUTIL_INCLUDE_DIR})

# Add the updater
add_subdirectory(mmc_updater)

# Add the GUI -> Logic connection header
add_subdirectory(depends/LogicalGui)
include_directories(${LOGICALGUI_INCLUDE_DIR})

# Add the GUI -> Logic connection header
add_subdirectory(depends/iconfix)
include_directories(${ICONFIX_INCLUDE_DIR})

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
	# Application base
	MultiMC.h
	MultiMC.cpp
	MMCError.h
	BuildConfig.h
	${PROJECT_BINARY_DIR}/BuildConfig.cpp

	# Logging
	logger/QsDebugOutput.cpp
	logger/QsDebugOutput.h
	logger/QsLog.cpp
	logger/QsLog.h
	logger/QsLogDest.cpp
	logger/QsLogDest.h

	# GUI - general utilities
	gui/GuiUtil.h
	gui/GuiUtil.cpp
	gui/ColumnResizer.h
	gui/ColumnResizer.cpp

	# GUI - windows
	gui/MainWindow.h
	gui/MainWindow.cpp
	gui/ConsoleWindow.h
	gui/ConsoleWindow.cpp

	# GUI - page dialog pages
	gui/pages/BasePage.h
	gui/pages/VersionPage.cpp
	gui/pages/VersionPage.h
	gui/pages/TexturePackPage.h
	gui/pages/ResourcePackPage.h
	gui/pages/ModFolderPage.cpp
	gui/pages/ModFolderPage.h
	gui/pages/NotesPage.cpp
	gui/pages/NotesPage.h
	gui/pages/LegacyUpgradePage.cpp
	gui/pages/LegacyUpgradePage.h
	gui/pages/LegacyJarModPage.cpp
	gui/pages/LegacyJarModPage.h
	gui/pages/LogPage.cpp
	gui/pages/LogPage.h
	gui/pages/InstanceSettingsPage.cpp
	gui/pages/InstanceSettingsPage.h
	gui/pages/ScreenshotsPage.cpp
	gui/pages/ScreenshotsPage.h
	gui/pages/OtherLogsPage.cpp
	gui/pages/OtherLogsPage.h

	# GUI - global settings pages
	gui/pages/global/AccountListPage.cpp
	gui/pages/global/AccountListPage.h
	gui/pages/global/ExternalToolsPage.cpp
	gui/pages/global/ExternalToolsPage.h
	gui/pages/global/JavaPage.cpp
	gui/pages/global/JavaPage.h
	gui/pages/global/MinecraftPage.cpp
	gui/pages/global/MinecraftPage.h
	gui/pages/global/MultiMCPage.cpp
	gui/pages/global/MultiMCPage.h
	gui/pages/global/ProxyPage.cpp
	gui/pages/global/ProxyPage.h

	# GUI - dialogs
	gui/dialogs/AboutDialog.cpp
	gui/dialogs/AboutDialog.h
	gui/dialogs/AccountSelectDialog.cpp
	gui/dialogs/AccountSelectDialog.h
	gui/dialogs/CopyInstanceDialog.cpp
	gui/dialogs/CopyInstanceDialog.h
	gui/dialogs/CustomMessageBox.cpp
	gui/dialogs/CustomMessageBox.h
	gui/dialogs/EditAccountDialog.cpp
	gui/dialogs/EditAccountDialog.h
	gui/dialogs/IconPickerDialog.cpp
	gui/dialogs/IconPickerDialog.h
	gui/dialogs/LoginDialog.cpp
	gui/dialogs/LoginDialog.h
	gui/dialogs/LwjglSelectDialog.cpp
	gui/dialogs/LwjglSelectDialog.h
	gui/dialogs/ModEditDialogCommon.cpp
	gui/dialogs/ModEditDialogCommon.h
	gui/dialogs/NewInstanceDialog.cpp
	gui/dialogs/NewInstanceDialog.h
	gui/dialogs/NotificationDialog.cpp
	gui/dialogs/NotificationDialog.h
	gui/pagedialog/PageDialog.cpp
	gui/pagedialog/PageDialog.h
	gui/dialogs/ProgressDialog.cpp
	gui/dialogs/ProgressDialog.h
	gui/dialogs/UpdateDialog.cpp
	gui/dialogs/UpdateDialog.h
	gui/dialogs/VersionSelectDialog.cpp
	gui/dialogs/VersionSelectDialog.h


	# GUI - widgets
	gui/widgets/Common.cpp
	gui/widgets/Common.h
	gui/widgets/IconLabel.cpp
	gui/widgets/IconLabel.h
	gui/widgets/LabeledToolButton.cpp
	gui/widgets/LabeledToolButton.h
	gui/widgets/LineSeparator.cpp
	gui/widgets/LineSeparator.h
	gui/widgets/MCModInfoFrame.cpp
	gui/widgets/MCModInfoFrame.h
	gui/widgets/ModListView.cpp
	gui/widgets/ModListView.h
	gui/widgets/PageContainer.cpp
	gui/widgets/PageContainer.h
	gui/widgets/PageContainer_p.h
	gui/widgets/ServerStatus.cpp
	gui/widgets/ServerStatus.h
	gui/widgets/VersionListView.cpp
	gui/widgets/VersionListView.h


	# GUI - instance group view
	gui/groupview/GroupedProxyModel.cpp
	gui/groupview/GroupedProxyModel.h
	gui/groupview/GroupView.cpp
	gui/groupview/GroupView.h
	gui/groupview/InstanceDelegate.cpp
	gui/groupview/InstanceDelegate.h
	gui/groupview/VisualGroup.cpp
	gui/groupview/VisualGroup.h

	# LOGIC - Base classes and infrastructure
	logic/BaseVersion.h
	logic/InstanceFactory.h
	logic/InstanceFactory.cpp
	logic/BaseInstance.h
	logic/BaseInstance.cpp
	logic/Mod.h
	logic/Mod.cpp
	logic/ModList.h
	logic/ModList.cpp

	# sets and maps for deciding based on versions
	logic/VersionFilterData.h
	logic/VersionFilterData.cpp

	# Instance launch
	logic/MinecraftProcess.h
	logic/MinecraftProcess.cpp

	# Annoying nag screen logic
	logic/NagUtils.h
	logic/NagUtils.cpp

	# Player skin utilities
	logic/SkinUtils.h
	logic/SkinUtils.cpp

	# misc model filter
	logic/EnabledItemFilter.h
	logic/EnabledItemFilter.cpp

	# JSON parsing helpers
	logic/MMCJson.h
	logic/MMCJson.cpp

	# RW lock protected map
	logic/RWStorage.h

	# A variable that has an implicit default value and keeps track of changes
	logic/DefaultVariable.h

	# network stuffs
	logic/net/NetAction.h
	logic/net/MD5EtagDownload.h
	logic/net/MD5EtagDownload.cpp
	logic/net/ByteArrayDownload.h
	logic/net/ByteArrayDownload.cpp
	logic/net/CacheDownload.h
	logic/net/CacheDownload.cpp
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
	logic/LegacyUpdate.h
	logic/LegacyUpdate.cpp

	# OneSix instances
	logic/OneSixUpdate.h
	logic/OneSixUpdate.cpp
	logic/OneSixInstance.h
	logic/OneSixInstance.cpp

	# a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
	logic/QObjectPtr.h

	# Common utils for instances
	logic/JarUtils.h
	logic/JarUtils.cpp

	# OneSix version json infrastructure
	logic/minecraft/GradleSpecifier.h
	logic/minecraft/InstanceVersion.cpp
	logic/minecraft/InstanceVersion.h
	logic/minecraft/JarMod.cpp
	logic/minecraft/JarMod.h
	logic/minecraft/MinecraftVersion.cpp
	logic/minecraft/MinecraftVersion.h
	logic/minecraft/MinecraftVersionList.cpp
	logic/minecraft/MinecraftVersionList.h
	logic/minecraft/OneSixLibrary.cpp
	logic/minecraft/OneSixLibrary.h
	logic/minecraft/OneSixRule.cpp
	logic/minecraft/OneSixRule.h
	logic/minecraft/OpSys.cpp
	logic/minecraft/OpSys.h
	logic/minecraft/ParseUtils.cpp
	logic/minecraft/ParseUtils.h
	logic/minecraft/RawLibrary.cpp
	logic/minecraft/RawLibrary.h
	logic/minecraft/VersionBuilder.cpp
	logic/minecraft/VersionBuilder.h
	logic/minecraft/VersionBuildError.h
	logic/minecraft/VersionFile.cpp
	logic/minecraft/VersionFile.h
	logic/minecraft/VersionPatch.h
	logic/minecraft/VersionSource.h

	# A Recursive file system watcher
	logic/RecursiveFileSystemWatcher.h
	logic/RecursiveFileSystemWatcher.cpp

	# Various base classes
	logic/BaseInstaller.h
	logic/BaseInstaller.cpp
	logic/BaseVersionList.h
	logic/BaseVersionList.cpp

	logic/InstanceList.h
	logic/InstanceList.cpp
	logic/LwjglVersionList.h
	logic/LwjglVersionList.cpp

	# FTB
	logic/OneSixFTBInstance.h
	logic/OneSixFTBInstance.cpp
	logic/LegacyFTBInstance.h
	logic/LegacyFTBInstance.cpp

	# the screenshots feature
	logic/screenshots/Screenshot.h
	logic/screenshots/ImgurUpload.h
	logic/screenshots/ImgurUpload.cpp
	logic/screenshots/ImgurAlbumCreation.h
	logic/screenshots/ImgurAlbumCreation.cpp

	# Icons
	logic/icons/MMCIcon.h
	logic/icons/MMCIcon.cpp
	logic/icons/IconList.h
	logic/icons/IconList.cpp

	# Tasks
	logic/tasks/ProgressProvider.h
	logic/tasks/Task.h
	logic/tasks/Task.cpp
	logic/tasks/ThreadTask.h
	logic/tasks/ThreadTask.cpp
	logic/tasks/SequentialTask.h
	logic/tasks/SequentialTask.cpp

	# Settings
	logic/settings/INIFile.cpp
	logic/settings/INIFile.h
	logic/settings/INISettingsObject.cpp
	logic/settings/INISettingsObject.h
	logic/settings/OverrideSetting.cpp
	logic/settings/OverrideSetting.h
	logic/settings/Setting.cpp
	logic/settings/Setting.h
	logic/settings/SettingsObject.cpp
	logic/settings/SettingsObject.h

	# Java related code
	logic/java/JavaChecker.h
	logic/java/JavaChecker.cpp
	logic/java/JavaUtils.h
	logic/java/JavaUtils.cpp
	logic/java/JavaVersionList.h
	logic/java/JavaVersionList.cpp
	logic/java/JavaCheckerJob.h
	logic/java/JavaCheckerJob.cpp

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

	# Forge and all things forge related
	logic/forge/ForgeVersion.h
	logic/forge/ForgeVersion.cpp
	logic/forge/ForgeVersionList.h
	logic/forge/ForgeVersionList.cpp
	logic/forge/ForgeMirror.h
	logic/forge/ForgeMirrors.h
	logic/forge/ForgeMirrors.cpp
	logic/forge/ForgeXzDownload.h
	logic/forge/ForgeXzDownload.cpp
	logic/forge/LegacyForge.h
	logic/forge/LegacyForge.cpp
	logic/forge/ForgeInstaller.h
	logic/forge/ForgeInstaller.cpp

	# Liteloader and related things
	logic/liteloader/LiteLoaderInstaller.h
	logic/liteloader/LiteLoaderInstaller.cpp
	logic/liteloader/LiteLoaderVersionList.h
	logic/liteloader/LiteLoaderVersionList.cpp

	# Translations
	logic/trans/TranslationDownloader.h
	logic/trans/TranslationDownloader.cpp
)


######## UIs ########
SET(MULTIMC_UIS
	# Option pages
	gui/pages/VersionPage.ui
	gui/pages/ModFolderPage.ui
	gui/pages/LegacyUpgradePage.ui
	gui/pages/LegacyJarModPage.ui
	gui/pages/LogPage.ui
	gui/pages/InstanceSettingsPage.ui
	gui/pages/NotesPage.ui
	gui/pages/ScreenshotsPage.ui
	gui/pages/OtherLogsPage.ui

	# Global settings pages
	gui/pages/global/AccountListPage.ui
	gui/pages/global/ExternalToolsPage.ui
	gui/pages/global/JavaPage.ui
	gui/pages/global/MinecraftPage.ui
	gui/pages/global/MultiMCPage.ui
	gui/pages/global/ProxyPage.ui

	# Dialogs
	gui/dialogs/CopyInstanceDialog.ui
	gui/dialogs/NewInstanceDialog.ui
	gui/dialogs/AboutDialog.ui
	gui/dialogs/VersionSelectDialog.ui
	gui/dialogs/LwjglSelectDialog.ui
	gui/dialogs/ProgressDialog.ui
	gui/dialogs/IconPickerDialog.ui
	gui/dialogs/AccountSelectDialog.ui
	gui/dialogs/EditAccountDialog.ui
	gui/dialogs/LoginDialog.ui
	gui/dialogs/UpdateDialog.ui
	gui/dialogs/NotificationDialog.ui

	# Widgets/other
	gui/widgets/MCModInfoFrame.ui
)

set(FILES_TO_TRANSLATE)
foreach(file ${MULTIMC_SOURCES})
	get_filename_component(absfile "${file}" ABSOLUTE)
	list(APPEND FILES_TO_TRANSLATE "${absfile}")
endforeach()

foreach(file ${MULTIMC_UIS})
	get_filename_component(absfile "${file}" ABSOLUTE)
	list(APPEND FILES_TO_TRANSLATE "${absfile}")
endforeach()

set(MULTIMC_QRCS
	resources/backgrounds/backgrounds.qrc
	resources/multimc/multimc.qrc
	resources/pe_dark/pe_dark.qrc
	resources/pe_light/pe_light.qrc
	resources/pe_colored/pe_colored.qrc
	resources/pe_blue/pe_blue.qrc
	resources/OSX/OSX.qrc
	resources/iOS/iOS.qrc
	resources/instances/instances.qrc
	resources/versions/versions.qrc
)


######## Windows resource files ########
if(WIN32)
	set(MULTIMC_RCS resources/multimc.rc)
endif()

####### X11 Stuff #######
if(UNIX AND NOT APPLE)
	find_package(Qt5X11Extras REQUIRED)
	set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} Qt5::X11Extras)
	set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} xcb)
	list(APPEND MULTIMC_SOURCES gui/Platform_X11.cpp)
else()
	list(APPEND MULTIMC_SOURCES gui/Platform_Other.cpp)
endif()


################################ COMPILE ################################

# Link additional libraries
if(WIN32)
	set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} Qt5::WinMain)
endif(WIN32)

# Tell CMake that MultiMCLauncher.jar is generated.
#SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/depends/launcher/MultiMCLauncher.jar GENERATED)
#SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/depends/javacheck/JavaCheck.jar GENERATED)

# Qt 5 stuff
qt5_wrap_ui(MULTIMC_UI ${MULTIMC_UIS})
qt5_add_resources(MULTIMC_RESOURCES ${MULTIMC_QRCS})

# Add common library
add_library(MultiMC_common STATIC ${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_RESOURCES})

# Add executable
add_executable(MultiMC MACOSX_BUNDLE WIN32 main.cpp ${MULTIMC_RCS})

# Link
target_link_libraries(MultiMC MultiMC_common)

target_link_libraries(MultiMC_common xz-embedded unpack200 iconfix libUtil LogicalGui
	${QUAZIP_LIBRARIES} Qt5::Core Qt5::Xml Qt5::Widgets Qt5::Network Qt5::Concurrent Qt5::WebKitWidgets
		${MultiMC_LINK_ADDITIONAL_LIBS}
)

target_link_libraries(MultiMC_common Qt5::Core Qt5::Widgets Qt5::Network Qt5::Concurrent Qt5::WebKitWidgets
	${MultiMC_LINK_ADDITIONAL_LIBS}
)

add_dependencies(MultiMC_common QuaZIP)

################################ INSTALLATION AND PACKAGING ################################

######## Install ########

#### Executable ####
if(APPLE AND UNIX) ## OSX
	install(TARGETS MultiMC
		BUNDLE DESTINATION . COMPONENT Runtime
		RUNTIME DESTINATION MultiMC.app/Contents/MacOS COMPONENT Runtime
	)

elseif(UNIX) ## LINUX and similar
	install(TARGETS MultiMC
		BUNDLE DESTINATION . COMPONENT Runtime
		RUNTIME DESTINATION bin COMPONENT Runtime
	)
	install(PROGRAMS package/linux/MultiMC DESTINATION .)

elseif(WIN32) ## WINDOWS
	install(TARGETS MultiMC
		BUNDLE DESTINATION . COMPONENT Runtime
		LIBRARY DESTINATION . COMPONENT Runtime
		RUNTIME DESTINATION . COMPONENT Runtime
	)
endif()

#### Dist package logic ####

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	# Image formats
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "tga|tiff|mng" EXCLUDE
	)
	# Icon engines
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/iconengines"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "fontawesome" EXCLUDE
	)
	# Platform plugins
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/platforms"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "minimal|linuxfb|offscreen" EXCLUDE
	)
else()
	# Image formats
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "tga|tiff|mng" EXCLUDE
		REGEX "d\\." EXCLUDE
		REGEX "_debug\\." EXCLUDE
	)
	# Icon engines
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/iconengines"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "fontawesome" EXCLUDE
		REGEX "d\\." EXCLUDE
		REGEX "_debug\\." EXCLUDE
	)
	# Platform plugins
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/platforms"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "minimal|linuxfb|offscreen" EXCLUDE
		REGEX "d\\." EXCLUDE
		REGEX "_debug\\." EXCLUDE
	)
	if(APPLE)
	# Accessible plugin to make buttons look decent on osx
	install(
		DIRECTORY "${QT_PLUGINS_DIR}/accessible"
		DESTINATION ${PLUGIN_DEST_DIR}
		COMPONENT Runtime
		REGEX "quick" EXCLUDE
		REGEX "d\\." EXCLUDE
		REGEX "_debug\\." EXCLUDE
	)
	endif()
endif()

# qtconf
install(
	CODE "
file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
"
	COMPONENT Runtime
)

# ICNS file for OS X
if(APPLE)
	install(FILES resources/MultiMC.icns DESTINATION MultiMC.app/Contents/Resources)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/install_prereqs.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake"
	@ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake" COMPONENT Runtime)



######## Package ########

# Package with CPack
if(UNIX)
	if(APPLE)
	set(CPACK_GENERATOR "ZIP")
	else()
	set(CPACK_GENERATOR "TGZ")
	endif()
elseif(WIN32)
	set(CPACK_GENERATOR "ZIP")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

set(CPACK_PACKAGE_NAME "MultiMC 5")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MultiMC - Minecraft launcher and management tool.")
set(CPACK_PACKAGE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
set(CPACK_PACKAGE_VERSION_MAJOR ${MultiMC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MultiMC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MultiMC_VERSION_REV})

if(CPACK_GENERATOR STREQUAL "NSIS")
	set(CPACK_PACKAGE_FILE_NAME "Setup-MultiMC")
else()
	set(CPACK_PACKAGE_FILE_NAME "MultiMC")
endif()

if(WIN32)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "MultiMC 5")
endif()

include(CPack)

include(Coverity)

# Translations
add_subdirectory(translations)

# Tests
add_subdirectory(tests)
