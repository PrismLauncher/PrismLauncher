project(application)

######## Configure the file with build properties ########
configure_file("${PROJECT_SOURCE_DIR}/BuildConfig.cpp.in" "${PROJECT_BINARY_DIR}/BuildConfig.cpp")

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
	HoeDown.h

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
	dialogs/NewComponentDialog.cpp
	dialogs/NewComponentDialog.h
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
	# Instance pages
	pages/VersionPage.ui
	pages/ModFolderPage.ui
	pages/LogPage.ui
	pages/InstanceSettingsPage.ui
	pages/NotesPage.ui
	pages/ScreenshotsPage.ui
	pages/OtherLogsPage.ui
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
	dialogs/NewComponentDialog.ui
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
	resources/flat/flat.qrc
	resources/documents/documents.qrc
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
if(DEFINED MultiMC_APP_BINARY_NAME)
	set_target_properties(MultiMC PROPERTIES OUTPUT_NAME "${MultiMC_APP_BINARY_NAME}")
endif()
if(DEFINED MultiMC_BINARY_RPATH)
	SET_TARGET_PROPERTIES(MultiMC PROPERTIES INSTALL_RPATH "${MultiMC_BINARY_RPATH}")
endif()
if(DEFINED MultiMC_APP_BINARY_DEFS)
	target_compile_definitions(MultiMC PRIVATE ${MultiMC_APP_BINARY_DEFS})
endif()

install(TARGETS MultiMC
	BUNDLE DESTINATION ${BUNDLE_DEST_DIR} COMPONENT Runtime
	LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
	RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
)

#### The MultiMC bundle mess! ####
# Bundle utilities are used to complete the portable packages - they add all the libraries that would otherwise be missing on the target system.
# NOTE: it seems that this absolutely has to be here, and nowhere else.
if(INSTALL_BUNDLE STREQUAL "full")
	# Add qt.conf - this makes Qt stop looking for things outside the bundle
	install(
		CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${RESOURCES_DEST_DIR}/qt.conf\" \" \")"
		COMPONENT Runtime
	)
	# Bundle plugins
	if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		# Image formats
		install(
			DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
			DESTINATION ${PLUGIN_DEST_DIR}
			COMPONENT Runtime
			REGEX "tga|tiff|mng|webp" EXCLUDE
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
			REGEX "tga|tiff|mng|webp" EXCLUDE
			REGEX "d\\." EXCLUDE
			REGEX "_debug\\." EXCLUDE
			REGEX "\\.dSYM" EXCLUDE
		)
		# Icon engines
		install(
			DIRECTORY "${QT_PLUGINS_DIR}/iconengines"
			DESTINATION ${PLUGIN_DEST_DIR}
			COMPONENT Runtime
			REGEX "fontawesome" EXCLUDE
			REGEX "d\\." EXCLUDE
			REGEX "_debug\\." EXCLUDE
			REGEX "\\.dSYM" EXCLUDE
		)
		# Platform plugins
		install(
			DIRECTORY "${QT_PLUGINS_DIR}/platforms"
			DESTINATION ${PLUGIN_DEST_DIR}
			COMPONENT Runtime
			REGEX "minimal|linuxfb|offscreen" EXCLUDE
			REGEX "d\\." EXCLUDE
			REGEX "_debug\\." EXCLUDE
			REGEX "\\.dSYM" EXCLUDE
		)
	endif()
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/install_prereqs.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake"
		@ONLY
	)
	install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake" COMPONENT Runtime)
endif()
