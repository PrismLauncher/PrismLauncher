project(MultiMC-Application)

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

#### Check the current Git commit
include(GitFunctions)
git_run(COMMAND rev-parse HEAD DEFAULT "Unknown" OUTPUT_VAR MultiMC_GIT_COMMIT)
message(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")

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

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MultiMC_VERSION_STRING}")

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
set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR})

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
	# Application base
	main.cpp
	MultiMC.h
	MultiMC.cpp
	BuildConfig.h
	${PROJECT_BINARY_DIR}/BuildConfig.cpp

	# GUI - general utilities
	GuiUtil.h
	GuiUtil.cpp
	ColumnResizer.h
	ColumnResizer.cpp
	InstanceProxyModel.h
	InstanceProxyModel.cpp
	VersionProxyModel.h
	VersionProxyModel.cpp

	# GUI - windows
	MainWindow.h
	MainWindow.cpp
	ConsoleWindow.h
	ConsoleWindow.cpp

	# page provider for instances
	InstancePageProvider.h
	InstancePageProvider.cpp

	# Annoying nag screen logic
	NagUtils.h
	NagUtils.cpp


	# GUI - page dialog pages
	pages/BasePage.h
	pages/VersionPage.cpp
	pages/VersionPage.h
	pages/TexturePackPage.h
	pages/ResourcePackPage.h
	pages/ModFolderPage.cpp
	pages/ModFolderPage.h
	pages/NotesPage.cpp
	pages/NotesPage.h
	pages/LogPage.cpp
	pages/LogPage.h
	pages/InstanceSettingsPage.cpp
	pages/InstanceSettingsPage.h
	pages/ScreenshotsPage.cpp
	pages/ScreenshotsPage.h
	pages/OtherLogsPage.cpp
	pages/OtherLogsPage.h
	pages/LegacyJarModPage.cpp
	pages/LegacyJarModPage.h
	pages/LegacyUpgradePage.cpp
	pages/LegacyUpgradePage.h

	# GUI - global settings pages
	pages/global/AccountListPage.cpp
	pages/global/AccountListPage.h
	pages/global/ExternalToolsPage.cpp
	pages/global/ExternalToolsPage.h
	pages/global/JavaPage.cpp
	pages/global/JavaPage.h
	pages/global/MinecraftPage.cpp
	pages/global/MinecraftPage.h
	pages/global/MultiMCPage.cpp
	pages/global/MultiMCPage.h
	pages/global/ProxyPage.cpp
	pages/global/ProxyPage.h

	# GUI - dialogs
	dialogs/AboutDialog.cpp
	dialogs/AboutDialog.h
	dialogs/AccountSelectDialog.cpp
	dialogs/AccountSelectDialog.h
	dialogs/CopyInstanceDialog.cpp
	dialogs/CopyInstanceDialog.h
	dialogs/CustomMessageBox.cpp
	dialogs/CustomMessageBox.h
	dialogs/EditAccountDialog.cpp
	dialogs/EditAccountDialog.h
	dialogs/ExportInstanceDialog.cpp
	dialogs/ExportInstanceDialog.h
	dialogs/IconPickerDialog.cpp
	dialogs/IconPickerDialog.h
	dialogs/LoginDialog.cpp
	dialogs/LoginDialog.h
	dialogs/ModEditDialogCommon.cpp
	dialogs/ModEditDialogCommon.h
	dialogs/NewInstanceDialog.cpp
	dialogs/NewInstanceDialog.h
	dialogs/NotificationDialog.cpp
	dialogs/NotificationDialog.h
	pagedialog/PageDialog.cpp
	pagedialog/PageDialog.h
	dialogs/ProgressDialog.cpp
	dialogs/ProgressDialog.h
	dialogs/UpdateDialog.cpp
	dialogs/UpdateDialog.h
	dialogs/VersionSelectDialog.cpp
	dialogs/VersionSelectDialog.h


	# GUI - widgets
	widgets/Common.cpp
	widgets/Common.h
	widgets/IconLabel.cpp
	widgets/IconLabel.h
	widgets/LabeledToolButton.cpp
	widgets/LabeledToolButton.h
	widgets/LineSeparator.cpp
	widgets/LineSeparator.h
	widgets/MCModInfoFrame.cpp
	widgets/MCModInfoFrame.h
	widgets/ModListView.cpp
	widgets/ModListView.h
	widgets/PageContainer.cpp
	widgets/PageContainer.h
	widgets/PageContainer_p.h
	widgets/ServerStatus.cpp
	widgets/ServerStatus.h
	widgets/VersionListView.cpp
	widgets/VersionListView.h


	# GUI - instance group view
	groupview/GroupedProxyModel.cpp
	groupview/GroupedProxyModel.h
	groupview/GroupView.cpp
	groupview/GroupView.h
	groupview/InstanceDelegate.cpp
	groupview/InstanceDelegate.h
	groupview/VisualGroup.cpp
	groupview/VisualGroup.h
)

######## UIs ########
SET(MULTIMC_UIS
	# Option pages
	pages/VersionPage.ui
	pages/ModFolderPage.ui
	pages/LogPage.ui
	pages/InstanceSettingsPage.ui
	pages/NotesPage.ui
	pages/ScreenshotsPage.ui
	pages/OtherLogsPage.ui
	pages/LegacyJarModPage.ui
	pages/LegacyUpgradePage.ui

	# Global settings pages
	pages/global/AccountListPage.ui
	pages/global/ExternalToolsPage.ui
	pages/global/JavaPage.ui
	pages/global/MinecraftPage.ui
	pages/global/MultiMCPage.ui
	pages/global/ProxyPage.ui

	# Dialogs
	dialogs/CopyInstanceDialog.ui
	dialogs/NewInstanceDialog.ui
	dialogs/AboutDialog.ui
	dialogs/VersionSelectDialog.ui
	dialogs/ProgressDialog.ui
	dialogs/IconPickerDialog.ui
	dialogs/AccountSelectDialog.ui
	dialogs/EditAccountDialog.ui
	dialogs/ExportInstanceDialog.ui
	dialogs/LoginDialog.ui
	dialogs/UpdateDialog.ui
	dialogs/NotificationDialog.ui

	# Widgets/other
	widgets/MCModInfoFrame.ui
)

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
	set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} xcb Qt5::X11Extras)
	list(APPEND MULTIMC_SOURCES Platform_X11.cpp)
else()
	list(APPEND MULTIMC_SOURCES Platform_Other.cpp)
endif()


# Link additional libraries
if(WIN32)
	set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} Qt5::WinMain)
endif(WIN32)

include_directories(../logic)

# Qt 5 stuff
qt5_wrap_ui(MULTIMC_UI ${MULTIMC_UIS})
qt5_add_resources(MULTIMC_RESOURCES ${MULTIMC_QRCS})

# Add executable
add_executable(MultiMC MACOSX_BUNDLE WIN32 ${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_RESOURCES} ${MULTIMC_RCS})
target_link_libraries(MultiMC MultiMC_logic xz-embedded unpack200 iconfix libUtil LogicalGui
	${QUAZIP_LIBRARIES} Qt5::Core Qt5::Xml Qt5::Widgets Qt5::Network Qt5::Concurrent Qt5::WebKitWidgets
		${MultiMC_LINK_ADDITIONAL_LIBS})

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

#### Java bits ####
install_jar(JavaCheck "${BINARY_DEST_DIR}/jars")
install_jar(NewLaunch "${BINARY_DEST_DIR}/jars")

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

set(CPACK_PACKAGE_FILE_NAME "MultiMC")

include(CPack)
