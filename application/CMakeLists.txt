project(application)

######## Set URLs ########
set(MultiMC_NEWS_RSS_URL "http://multimc.org/rss.xml" CACHE STRING "URL to fetch MultiMC's news RSS feed from.")

######## Set version numbers ########
set(MultiMC_VERSION_MAJOR	 0)
set(MultiMC_VERSION_MINOR	 5)
set(MultiMC_VERSION_HOTFIX   0)

# Build number
set(MultiMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Build platform.
set(MultiMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Channel list URL
set(MultiMC_CHANLIST_URL "" CACHE STRING "URL for the channel list.")

# Notification URL
set(MultiMC_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

# paste.ee API key
set(MultiMC_PASTE_EE_API_KEY "" CACHE STRING "API key you can get from paste.ee when you register an account")

# Google analytics ID
set(MultiMC_ANALYTICS_ID "" CACHE STRING "ID you can get from Google analytics")

#### Check the current Git commit and branch
include(GetGitRevisionDescription)
get_git_head_revision(MultiMC_GIT_REFSPEC MultiMC_GIT_COMMIT)

message(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")
message(STATUS "Git refspec: ${MultiMC_GIT_REFSPEC}")

set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_HOTFIX}")

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MultiMC_RELEASE_VERSION_NAME}")

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
	set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2015-2017 MultiMC Contributors")
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
set(DIRS ${QT_LIBS_DIR} ${QT_LIBEXECS_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
	# Application base
	main.cpp
	MultiMC.h
	MultiMC.cpp
	BuildConfig.h
	${PROJECT_BINARY_DIR}/BuildConfig.cpp
	UpdateController.cpp
	UpdateController.h

	# GUI - general utilities
	GuiUtil.h
	GuiUtil.cpp
	ColumnResizer.h
	ColumnResizer.cpp
	InstanceProxyModel.h
	InstanceProxyModel.cpp
	VersionProxyModel.h
	VersionProxyModel.cpp
	ColorCache.h
	ColorCache.cpp

	# GUI - windows
	MainWindow.h
	MainWindow.cpp
	InstanceWindow.h
	InstanceWindow.cpp

	# GUI - setup wizard
	setupwizard/SetupWizard.h
	setupwizard/SetupWizard.cpp
	setupwizard/AnalyticsWizardPage.cpp
	setupwizard/AnalyticsWizardPage.h
	setupwizard/BaseWizardPage.h
	setupwizard/JavaWizardPage.cpp
	setupwizard/JavaWizardPage.h
	setupwizard/LanguageWizardPage.cpp
	setupwizard/LanguageWizardPage.h

	# GUI - themes
	themes/FusionTheme.cpp
	themes/FusionTheme.h
	themes/BrightTheme.cpp
	themes/BrightTheme.h
	themes/CustomTheme.cpp
	themes/CustomTheme.h
	themes/DarkTheme.cpp
	themes/DarkTheme.h
	themes/ITheme.cpp
	themes/ITheme.h
	themes/SystemTheme.cpp
	themes/SystemTheme.h

	# GUI - settings-specific wrappers for paged dialog
	SettingsUI.h

	# Processes
	LaunchController.h
	LaunchController.cpp

	# page provider for instances
	InstancePageProvider.h
	InstancePageProvider.cpp

	# Common java checking UI
	JavaCommon.h
	JavaCommon.cpp

	# GUI - page dialog pages
	pages/BasePage.h
	pages/BasePageContainer.h
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
	pages/WorldListPage.cpp
	pages/WorldListPage.h

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
	pages/global/PasteEEPage.cpp
	pages/global/PasteEEPage.h
	pages/global/PackagesPage.cpp
	pages/global/PackagesPage.h

	# GUI - dialogs
	dialogs/AboutDialog.cpp
	dialogs/AboutDialog.h
	dialogs/ProfileSelectDialog.cpp
	dialogs/ProfileSelectDialog.h
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
	dialogs/SkinUploadDialog.cpp
	dialogs/SkinUploadDialog.h

	# GUI - widgets
	widgets/Common.cpp
	widgets/Common.h
	widgets/FocusLineEdit.cpp
	widgets/FocusLineEdit.h
	widgets/IconLabel.cpp
	widgets/IconLabel.h
	widgets/LabeledToolButton.cpp
	widgets/LabeledToolButton.h
	widgets/LineSeparator.cpp
	widgets/LineSeparator.h
	widgets/LogView.cpp
	widgets/LogView.h
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
	widgets/VersionSelectWidget.cpp
	widgets/VersionSelectWidget.h
	widgets/ProgressWidget.h
	widgets/ProgressWidget.cpp


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
	pages/WorldListPage.ui

	# Global settings pages
	pages/global/AccountListPage.ui
	pages/global/ExternalToolsPage.ui
	pages/global/JavaPage.ui
	pages/global/MinecraftPage.ui
	pages/global/MultiMCPage.ui
	pages/global/ProxyPage.ui
	pages/global/PasteEEPage.ui
	pages/global/PackagesPage.ui

	# Dialogs
	dialogs/CopyInstanceDialog.ui
	dialogs/NewInstanceDialog.ui
	dialogs/AboutDialog.ui
	dialogs/ProgressDialog.ui
	dialogs/IconPickerDialog.ui
	dialogs/ProfileSelectDialog.ui
	dialogs/EditAccountDialog.ui
	dialogs/ExportInstanceDialog.ui
	dialogs/LoginDialog.ui
	dialogs/UpdateDialog.ui
	dialogs/NotificationDialog.ui
	dialogs/SkinUploadDialog.ui

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
)

######## Windows resource files ########
if(WIN32)
	set(MULTIMC_RCS resources/multimc.rc)
endif()

# Qt 5 stuff
qt5_wrap_ui(MULTIMC_UI ${MULTIMC_UIS})
qt5_add_resources(MULTIMC_RESOURCES ${MULTIMC_QRCS})

# Add executable
add_executable(MultiMC MACOSX_BUNDLE WIN32 ${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_RESOURCES} ${MULTIMC_RCS})
target_link_libraries(MultiMC MultiMC_gui ${QUAZIP_LIBRARIES} hoedown MultiMC_rainbow LocalPeer ganalytics)

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
endif()

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
