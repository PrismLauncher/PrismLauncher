project(application)

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
    # Application base
    main.cpp
    MultiMC.h
    MultiMC.cpp
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

    # Super secret!
    KonamiCode.h
    KonamiCode.cpp

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

    # Processes
    LaunchController.h
    LaunchController.cpp

    # page provider for instances
    InstancePageProvider.h

    # Common java checking UI
    JavaCommon.h
    JavaCommon.cpp

    # GUI - paged dialog base
    pages/BasePage.h
    pages/BasePageContainer.h
    pages/BasePageProvider.h

    # GUI - instance pages
    pages/instance/GameOptionsPage.cpp
    pages/instance/GameOptionsPage.h
    pages/instance/VersionPage.cpp
    pages/instance/VersionPage.h
    pages/instance/TexturePackPage.h
    pages/instance/ResourcePackPage.h
    pages/instance/ModFolderPage.cpp
    pages/instance/ModFolderPage.h
    pages/instance/NotesPage.cpp
    pages/instance/NotesPage.h
    pages/instance/LogPage.cpp
    pages/instance/LogPage.h
    pages/instance/InstanceSettingsPage.cpp
    pages/instance/InstanceSettingsPage.h
    pages/instance/ScreenshotsPage.cpp
    pages/instance/ScreenshotsPage.h
    pages/instance/OtherLogsPage.cpp
    pages/instance/OtherLogsPage.h
    pages/instance/ServersPage.cpp
    pages/instance/ServersPage.h
    pages/instance/LegacyUpgradePage.cpp
    pages/instance/LegacyUpgradePage.h
    pages/instance/WorldListPage.cpp
    pages/instance/WorldListPage.h

    # GUI - global settings pages
    pages/global/AccountListPage.cpp
    pages/global/AccountListPage.h
    pages/global/CustomCommandsPage.cpp
    pages/global/CustomCommandsPage.h
    pages/global/ExternalToolsPage.cpp
    pages/global/ExternalToolsPage.h
    pages/global/JavaPage.cpp
    pages/global/JavaPage.h
    pages/global/LanguagePage.cpp
    pages/global/LanguagePage.h
    pages/global/MinecraftPage.cpp
    pages/global/MinecraftPage.h
    pages/global/MultiMCPage.cpp
    pages/global/MultiMCPage.h
    pages/global/ProxyPage.cpp
    pages/global/ProxyPage.h
    pages/global/PasteEEPage.cpp
    pages/global/PasteEEPage.h

    # GUI - platform pages
    pages/modplatform/VanillaPage.cpp
    pages/modplatform/VanillaPage.h
    pages/modplatform/ftb/FtbFilterModel.cpp
    pages/modplatform/ftb/FtbFilterModel.h
    pages/modplatform/ftb/FtbListModel.cpp
    pages/modplatform/ftb/FtbListModel.h
    pages/modplatform/ftb/FtbPage.cpp
    pages/modplatform/ftb/FtbPage.h
    pages/modplatform/legacy_ftb/Page.cpp
    pages/modplatform/legacy_ftb/Page.h
    pages/modplatform/legacy_ftb/ListModel.h
    pages/modplatform/legacy_ftb/ListModel.cpp
    pages/modplatform/twitch/TwitchData.h
    pages/modplatform/twitch/TwitchModel.cpp
    pages/modplatform/twitch/TwitchModel.h
    pages/modplatform/twitch/TwitchPage.cpp
    pages/modplatform/twitch/TwitchPage.h
    pages/modplatform/technic/TechnicModel.cpp
    pages/modplatform/technic/TechnicModel.h
    pages/modplatform/technic/TechnicPage.cpp
    pages/modplatform/technic/TechnicPage.h
    pages/modplatform/ImportPage.cpp
    pages/modplatform/ImportPage.h

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
    widgets/CustomCommands.cpp
    widgets/CustomCommands.h
    widgets/DropLabel.cpp
    widgets/DropLabel.h
    widgets/FocusLineEdit.cpp
    widgets/FocusLineEdit.h
    widgets/IconLabel.cpp
    widgets/IconLabel.h
    widgets/JavaSettingsWidget.cpp
    widgets/JavaSettingsWidget.h
    widgets/LabeledToolButton.cpp
    widgets/LabeledToolButton.h
    widgets/LanguageSelectionWidget.cpp
    widgets/LanguageSelectionWidget.h
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
    widgets/WideBar.h
    widgets/WideBar.cpp

    # GUI - instance group view
    groupview/GroupedProxyModel.cpp
    groupview/GroupedProxyModel.h
    groupview/AccessibleGroupView.cpp
    groupview/AccessibleGroupView.h
    groupview/AccessibleGroupView_p.h
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
    pages/instance/GameOptionsPage.ui
    pages/instance/VersionPage.ui
    pages/instance/ModFolderPage.ui
    pages/instance/LogPage.ui
    pages/instance/InstanceSettingsPage.ui
    pages/instance/NotesPage.ui
    pages/instance/ScreenshotsPage.ui
    pages/instance/OtherLogsPage.ui
    pages/instance/LegacyUpgradePage.ui
    pages/instance/ServersPage.ui
    pages/instance/WorldListPage.ui

    # Global settings pages
    pages/global/AccountListPage.ui
    pages/global/ExternalToolsPage.ui
    pages/global/JavaPage.ui
    pages/global/MinecraftPage.ui
    pages/global/MultiMCPage.ui
    pages/global/ProxyPage.ui
    pages/global/PasteEEPage.ui

    # Platform pages
    pages/modplatform/VanillaPage.ui
    pages/modplatform/ftb/FtbPage.ui
    pages/modplatform/legacy_ftb/Page.ui
    pages/modplatform/twitch/TwitchPage.ui
    pages/modplatform/technic/TechnicPage.ui
    pages/modplatform/ImportPage.ui

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
    widgets/CustomCommands.ui
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
