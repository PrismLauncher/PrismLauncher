project(application)

################################ FILES ################################

######## Sources and headers ########

include (UnitTest)

set(CORE_SOURCES
    # LOGIC - Base classes and infrastructure
    BaseInstaller.h
    BaseInstaller.cpp
    BaseVersionList.h
    BaseVersionList.cpp
    InstanceList.h
    InstanceList.cpp
    InstanceTask.h
    InstanceTask.cpp
    LoggedProcess.h
    LoggedProcess.cpp
    MessageLevel.cpp
    MessageLevel.h
    BaseVersion.h
    BaseInstance.h
    BaseInstance.cpp
    NullInstance.h
    MMCZip.h
    MMCZip.cpp
    MMCStrings.h
    MMCStrings.cpp

    # Basic instance manipulation tasks (derived from InstanceTask)
    InstanceCreationTask.h
    InstanceCreationTask.cpp
    InstanceCopyTask.h
    InstanceCopyTask.cpp
    InstanceImportTask.h
    InstanceImportTask.cpp

    # Use tracking separate from memory management
    Usable.h

    # Prefix tree where node names are strings between separators
    SeparatorPrefixTree.h

    # String filters
    Filter.h
    Filter.cpp

    # JSON parsing helpers
    Json.h
    Json.cpp

    FileSystem.h
    FileSystem.cpp

    Exception.h

    # RW lock protected map
    RWStorage.h

    # A variable that has an implicit default value and keeps track of changes
    DefaultVariable.h

    # a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
    QObjectPtr.h

    # Compression support
    GZip.h
    GZip.cpp

    # Command line parameter parsing
    Commandline.h
    Commandline.cpp

    # Version number string support
    Version.h
    Version.cpp

    # A Recursive file system watcher
    RecursiveFileSystemWatcher.h
    RecursiveFileSystemWatcher.cpp

    # Time
    MMCTime.h
    MMCTime.cpp
)

add_unit_test(FileSystem
    SOURCES FileSystem_test.cpp
    LIBS Launcher_logic
    DATA testdata
    )

add_unit_test(GZip
    SOURCES GZip_test.cpp
    LIBS Launcher_logic
    )

set(PATHMATCHER_SOURCES
    # Path matchers
    pathmatcher/FSTreeMatcher.h
    pathmatcher/IPathMatcher.h
    pathmatcher/MultiMatcher.h
    pathmatcher/RegexpMatcher.h
)

set(NET_SOURCES
    # network stuffs
    net/ByteArraySink.h
    net/ChecksumValidator.h
    net/Download.cpp
    net/Download.h
    net/FileSink.cpp
    net/FileSink.h
    net/HttpMetaCache.cpp
    net/HttpMetaCache.h
    net/MetaCacheSink.cpp
    net/MetaCacheSink.h
    net/NetAction.h
    net/NetJob.cpp
    net/NetJob.h
    net/PasteUpload.cpp
    net/PasteUpload.h
    net/Sink.h
    net/Validator.h
)

# Game launch logic
set(LAUNCH_SOURCES
    launch/steps/LookupServerAddress.cpp
    launch/steps/LookupServerAddress.h
    launch/steps/PostLaunchCommand.cpp
    launch/steps/PostLaunchCommand.h
    launch/steps/PreLaunchCommand.cpp
    launch/steps/PreLaunchCommand.h
    launch/steps/TextPrint.cpp
    launch/steps/TextPrint.h
    launch/steps/Update.cpp
    launch/steps/Update.h
    launch/LaunchStep.cpp
    launch/LaunchStep.h
    launch/LaunchTask.cpp
    launch/LaunchTask.h
    launch/LogModel.cpp
    launch/LogModel.h
)

# Old update system
set(UPDATE_SOURCES
    updater/GoUpdate.h
    updater/GoUpdate.cpp
    updater/UpdateChecker.h
    updater/UpdateChecker.cpp
    updater/DownloadTask.h
    updater/DownloadTask.cpp
)

add_unit_test(UpdateChecker
    SOURCES updater/UpdateChecker_test.cpp
    LIBS Launcher_logic
    DATA updater/testdata
    )

add_unit_test(DownloadTask
    SOURCES updater/DownloadTask_test.cpp
    LIBS Launcher_logic
    DATA updater/testdata
    )

# Rarely used notifications
set(NOTIFICATIONS_SOURCES
    # Notifications - short warning messages
    notifications/NotificationChecker.h
    notifications/NotificationChecker.cpp
)

# Backend for the news bar... there's usually no news.
set(NEWS_SOURCES
    # News System
    news/NewsChecker.h
    news/NewsChecker.cpp
    news/NewsEntry.h
    news/NewsEntry.cpp
)

# Icon interface
set(ICONS_SOURCES
    # Icons System and related code
    icons/IconUtils.h
    icons/IconUtils.cpp
)

# Support for Minecraft instances and launch
set(MINECRAFT_SOURCES
    # Minecraft support
    minecraft/auth/AccountData.h
    minecraft/auth/AccountData.cpp
    minecraft/auth/AccountTask.h
    minecraft/auth/AccountTask.cpp
    minecraft/auth/AuthSession.h
    minecraft/auth/AuthSession.cpp
    minecraft/auth/AccountList.h
    minecraft/auth/AccountList.cpp
    minecraft/auth/MinecraftAccount.h
    minecraft/auth/MinecraftAccount.cpp
    minecraft/auth/flows/AuthContext.h
    minecraft/auth/flows/AuthContext.cpp
    minecraft/auth/flows/AuthRequest.h
    minecraft/auth/flows/AuthRequest.cpp

    minecraft/auth/flows/MSAInteractive.h
    minecraft/auth/flows/MSAInteractive.cpp
    minecraft/auth/flows/MSASilent.h
    minecraft/auth/flows/MSASilent.cpp

    minecraft/auth/flows/MojangLogin.h
    minecraft/auth/flows/MojangLogin.cpp
    minecraft/auth/flows/MojangRefresh.h
    minecraft/auth/flows/MojangRefresh.cpp

    minecraft/auth/flows/Yggdrasil.h
    minecraft/auth/flows/Yggdrasil.cpp

    minecraft/auth/flows/Parsers.h
    minecraft/auth/flows/Parsers.cpp

    minecraft/gameoptions/GameOptions.h
    minecraft/gameoptions/GameOptions.cpp

    minecraft/update/AssetUpdateTask.h
    minecraft/update/AssetUpdateTask.cpp
    minecraft/update/FMLLibrariesTask.cpp
    minecraft/update/FMLLibrariesTask.h
    minecraft/update/FoldersTask.cpp
    minecraft/update/FoldersTask.h
    minecraft/update/LibrariesTask.cpp
    minecraft/update/LibrariesTask.h

    minecraft/launch/ClaimAccount.cpp
    minecraft/launch/ClaimAccount.h
    minecraft/launch/CreateGameFolders.cpp
    minecraft/launch/CreateGameFolders.h
    minecraft/launch/ModMinecraftJar.cpp
    minecraft/launch/ModMinecraftJar.h
    minecraft/launch/DirectJavaLaunch.cpp
    minecraft/launch/DirectJavaLaunch.h
    minecraft/launch/ExtractNatives.cpp
    minecraft/launch/ExtractNatives.h
    minecraft/launch/LauncherPartLaunch.cpp
    minecraft/launch/LauncherPartLaunch.h
    minecraft/launch/MinecraftServerTarget.cpp
    minecraft/launch/MinecraftServerTarget.h
    minecraft/launch/PrintInstanceInfo.cpp
    minecraft/launch/PrintInstanceInfo.h
    minecraft/launch/ReconstructAssets.cpp
    minecraft/launch/ReconstructAssets.h
    minecraft/launch/ScanModFolders.cpp
    minecraft/launch/ScanModFolders.h
    minecraft/launch/VerifyJavaInstall.cpp
    minecraft/launch/VerifyJavaInstall.h

    minecraft/legacy/LegacyModList.h
    minecraft/legacy/LegacyModList.cpp
    minecraft/legacy/LegacyInstance.h
    minecraft/legacy/LegacyInstance.cpp
    minecraft/legacy/LegacyUpgradeTask.h
    minecraft/legacy/LegacyUpgradeTask.cpp

    minecraft/GradleSpecifier.h
    minecraft/MinecraftInstance.cpp
    minecraft/MinecraftInstance.h
    minecraft/LaunchProfile.cpp
    minecraft/LaunchProfile.h
    minecraft/Component.cpp
    minecraft/Component.h
    minecraft/PackProfile.cpp
    minecraft/PackProfile.h
    minecraft/ComponentUpdateTask.cpp
    minecraft/ComponentUpdateTask.h
    minecraft/MinecraftLoadAndCheck.h
    minecraft/MinecraftLoadAndCheck.cpp
    minecraft/MinecraftUpdate.h
    minecraft/MinecraftUpdate.cpp
    minecraft/MojangVersionFormat.cpp
    minecraft/MojangVersionFormat.h
    minecraft/Rule.cpp
    minecraft/Rule.h
    minecraft/OneSixVersionFormat.cpp
    minecraft/OneSixVersionFormat.h
    minecraft/OpSys.cpp
    minecraft/OpSys.h
    minecraft/ParseUtils.cpp
    minecraft/ParseUtils.h
    minecraft/ProfileUtils.cpp
    minecraft/ProfileUtils.h
    minecraft/Library.cpp
    minecraft/Library.h
    minecraft/MojangDownloadInfo.h
    minecraft/VersionFile.cpp
    minecraft/VersionFile.h
    minecraft/VersionFilterData.h
    minecraft/VersionFilterData.cpp
    minecraft/World.h
    minecraft/World.cpp
    minecraft/WorldList.h
    minecraft/WorldList.cpp

    minecraft/mod/Mod.h
    minecraft/mod/Mod.cpp
    minecraft/mod/ModDetails.h
    minecraft/mod/ModFolderModel.h
    minecraft/mod/ModFolderModel.cpp
    minecraft/mod/ModFolderLoadTask.h
    minecraft/mod/ModFolderLoadTask.cpp
    minecraft/mod/LocalModParseTask.h
    minecraft/mod/LocalModParseTask.cpp
    minecraft/mod/ResourcePackFolderModel.h
    minecraft/mod/ResourcePackFolderModel.cpp
    minecraft/mod/TexturePackFolderModel.h
    minecraft/mod/TexturePackFolderModel.cpp

    # Assets
    minecraft/AssetsUtils.h
    minecraft/AssetsUtils.cpp

    # Minecraft services
    minecraft/services/CapeChange.cpp
    minecraft/services/CapeChange.h
    minecraft/services/SkinUpload.cpp
    minecraft/services/SkinUpload.h
    minecraft/services/SkinDelete.cpp
    minecraft/services/SkinDelete.h

    mojang/PackageManifest.h
    mojang/PackageManifest.cpp
    )

add_unit_test(GradleSpecifier
    SOURCES minecraft/GradleSpecifier_test.cpp
    LIBS Launcher_logic
    )

add_executable(PackageManifest
    mojang/PackageManifest_test.cpp
)
target_link_libraries(PackageManifest
    Launcher_logic
    Qt5::Test
)
target_include_directories(PackageManifest
    PRIVATE ../cmake/UnitTest/
)
add_test(
    NAME PackageManifest
    COMMAND PackageManifest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_unit_test(MojangVersionFormat
    SOURCES minecraft/MojangVersionFormat_test.cpp
    LIBS Launcher_logic
    DATA minecraft/testdata
    )

add_unit_test(Library
    SOURCES minecraft/Library_test.cpp
    LIBS Launcher_logic
    )

# FIXME: shares data with FileSystem test
add_unit_test(ModFolderModel
    SOURCES minecraft/mod/ModFolderModel_test.cpp
    DATA testdata
    LIBS Launcher_logic
    )

add_unit_test(ParseUtils
    SOURCES minecraft/ParseUtils_test.cpp
    LIBS Launcher_logic
    )

# the screenshots feature
set(SCREENSHOTS_SOURCES
    screenshots/Screenshot.h
    screenshots/ImgurUpload.h
    screenshots/ImgurUpload.cpp
    screenshots/ImgurAlbumCreation.h
    screenshots/ImgurAlbumCreation.cpp
)

set(TASKS_SOURCES
    # Tasks
    tasks/Task.h
    tasks/Task.cpp
    tasks/SequentialTask.h
    tasks/SequentialTask.cpp
)

set(SETTINGS_SOURCES
    # Settings
    settings/INIFile.cpp
    settings/INIFile.h
    settings/INISettingsObject.cpp
    settings/INISettingsObject.h
    settings/OverrideSetting.cpp
    settings/OverrideSetting.h
    settings/PassthroughSetting.cpp
    settings/PassthroughSetting.h
    settings/Setting.cpp
    settings/Setting.h
    settings/SettingsObject.cpp
    settings/SettingsObject.h
)

add_unit_test(INIFile
    SOURCES settings/INIFile_test.cpp
    LIBS Launcher_logic
    )

set(JAVA_SOURCES
    # Java related code
    java/launch/CheckJava.cpp
    java/launch/CheckJava.h
    java/JavaChecker.h
    java/JavaChecker.cpp
    java/JavaCheckerJob.h
    java/JavaCheckerJob.cpp
    java/JavaInstall.h
    java/JavaInstall.cpp
    java/JavaInstallList.h
    java/JavaInstallList.cpp
    java/JavaUtils.h
    java/JavaUtils.cpp
    java/JavaVersion.h
    java/JavaVersion.cpp
)

add_unit_test(JavaVersion
    SOURCES java/JavaVersion_test.cpp
    LIBS Launcher_logic
    )

set(TRANSLATIONS_SOURCES
    translations/TranslationsModel.h
    translations/TranslationsModel.cpp
    translations/POTranslator.h
    translations/POTranslator.cpp
)

set(TOOLS_SOURCES
    # Tools
    tools/BaseExternalTool.cpp
    tools/BaseExternalTool.h
    tools/BaseProfiler.cpp
    tools/BaseProfiler.h
    tools/JProfiler.cpp
    tools/JProfiler.h
    tools/JVisualVM.cpp
    tools/JVisualVM.h
    tools/MCEditTool.cpp
    tools/MCEditTool.h
)

set(META_SOURCES
    # Metadata sources
    meta/JsonFormat.cpp
    meta/JsonFormat.h
    meta/BaseEntity.cpp
    meta/BaseEntity.h
    meta/VersionList.cpp
    meta/VersionList.h
    meta/Version.cpp
    meta/Version.h
    meta/Index.cpp
    meta/Index.h
)

set(FTB_SOURCES
    modplatform/legacy_ftb/PackFetchTask.h
    modplatform/legacy_ftb/PackFetchTask.cpp
    modplatform/legacy_ftb/PackInstallTask.h
    modplatform/legacy_ftb/PackInstallTask.cpp
    modplatform/legacy_ftb/PrivatePackManager.h
    modplatform/legacy_ftb/PrivatePackManager.cpp

    modplatform/legacy_ftb/PackHelpers.h
)

set(FLAME_SOURCES
    # Flame
    modplatform/flame/FlamePackIndex.cpp
    modplatform/flame/FlamePackIndex.h
    modplatform/flame/PackManifest.h
    modplatform/flame/PackManifest.cpp
    modplatform/flame/FileResolvingTask.h
    modplatform/flame/FileResolvingTask.cpp
)

set(MODPACKSCH_SOURCES
    modplatform/modpacksch/FTBPackInstallTask.h
    modplatform/modpacksch/FTBPackInstallTask.cpp
    modplatform/modpacksch/FTBPackManifest.h
    modplatform/modpacksch/FTBPackManifest.cpp
)

set(TECHNIC_SOURCES
    modplatform/technic/SingleZipPackInstallTask.h
    modplatform/technic/SingleZipPackInstallTask.cpp
    modplatform/technic/SolderPackInstallTask.h
    modplatform/technic/SolderPackInstallTask.cpp
    modplatform/technic/TechnicPackProcessor.h
    modplatform/technic/TechnicPackProcessor.cpp
)

set(ATLAUNCHER_SOURCES
    modplatform/atlauncher/ATLPackIndex.cpp
    modplatform/atlauncher/ATLPackIndex.h
    modplatform/atlauncher/ATLPackInstallTask.cpp
    modplatform/atlauncher/ATLPackInstallTask.h
    modplatform/atlauncher/ATLPackManifest.cpp
    modplatform/atlauncher/ATLPackManifest.h
)

add_unit_test(Index
    SOURCES meta/Index_test.cpp
    LIBS Launcher_logic
    )

################################ COMPILE ################################

# we need zlib
find_package(ZLIB REQUIRED)

set(LOGIC_SOURCES
    ${CORE_SOURCES}
    ${PATHMATCHER_SOURCES}
    ${NET_SOURCES}
    ${LAUNCH_SOURCES}
    ${UPDATE_SOURCES}
    ${NOTIFICATIONS_SOURCES}
    ${NEWS_SOURCES}
    ${MINECRAFT_SOURCES}
    ${SCREENSHOTS_SOURCES}
    ${TASKS_SOURCES}
    ${SETTINGS_SOURCES}
    ${JAVA_SOURCES}
    ${TRANSLATIONS_SOURCES}
    ${TOOLS_SOURCES}
    ${META_SOURCES}
    ${ICONS_SOURCES}
    ${FTB_SOURCES}
    ${FLAME_SOURCES}
    ${MODPACKSCH_SOURCES}
    ${TECHNIC_SOURCES}
    ${ATLAUNCHER_SOURCES}
)

SET(LAUNCHER_SOURCES
    # Application base
    Application.h
    Application.cpp
    UpdateController.cpp
    UpdateController.h
    ApplicationMessage.h
    ApplicationMessage.cpp

    # GUI - general utilities
    DesktopServices.h
    DesktopServices.cpp
    GuiUtil.h
    GuiUtil.cpp
    ColumnResizer.h
    ColumnResizer.cpp
    VersionProxyModel.h
    VersionProxyModel.cpp
    ColorCache.h
    ColorCache.cpp
    HoeDown.h

    # Super secret!
    KonamiCode.h
    KonamiCode.cpp

    # Icons
    icons/MMCIcon.h
    icons/MMCIcon.cpp
    icons/IconList.h
    icons/IconList.cpp

    # GUI - windows
    MainWindow.h
    MainWindow.cpp
    InstanceWindow.h
    InstanceWindow.cpp

    # FIXME: maybe find a better home for this.
    SkinUtils.cpp
    SkinUtils.h

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
    pages/instance/ShaderPackPage.h
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
    pages/global/LauncherPage.cpp
    pages/global/LauncherPage.h
    pages/global/ProxyPage.cpp
    pages/global/ProxyPage.h
    pages/global/PasteEEPage.cpp
    pages/global/PasteEEPage.h

    # GUI - platform pages
    pages/modplatform/VanillaPage.cpp
    pages/modplatform/VanillaPage.h

    pages/modplatform/atlauncher/AtlFilterModel.cpp
    pages/modplatform/atlauncher/AtlFilterModel.h
    pages/modplatform/atlauncher/AtlListModel.cpp
    pages/modplatform/atlauncher/AtlListModel.h
    pages/modplatform/atlauncher/AtlOptionalModDialog.cpp
    pages/modplatform/atlauncher/AtlOptionalModDialog.h
    pages/modplatform/atlauncher/AtlPage.cpp
    pages/modplatform/atlauncher/AtlPage.h

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

    pages/modplatform/flame/FlameModel.cpp
    pages/modplatform/flame/FlameModel.h
    pages/modplatform/flame/FlamePage.cpp
    pages/modplatform/flame/FlamePage.h

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
    dialogs/ProfileSetupDialog.cpp
    dialogs/ProfileSetupDialog.h
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
    dialogs/MSALoginDialog.cpp
    dialogs/MSALoginDialog.h
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
    widgets/VersionListView.cpp
    widgets/VersionListView.h
    widgets/VersionSelectWidget.cpp
    widgets/VersionSelectWidget.h
    widgets/ProgressWidget.h
    widgets/ProgressWidget.cpp
    widgets/WideBar.h
    widgets/WideBar.cpp

    # GUI - instance group view
    instanceview/InstanceProxyModel.cpp
    instanceview/InstanceProxyModel.h
    instanceview/AccessibleInstanceView.cpp
    instanceview/AccessibleInstanceView.h
    instanceview/AccessibleInstanceView_p.h
    instanceview/InstanceView.cpp
    instanceview/InstanceView.h
    instanceview/InstanceDelegate.cpp
    instanceview/InstanceDelegate.h
    instanceview/VisualGroup.cpp
    instanceview/VisualGroup.h
    )

######## UIs ########
SET(LAUNCHER_UIS
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
    pages/global/LauncherPage.ui
    pages/global/ProxyPage.ui
    pages/global/PasteEEPage.ui

    # Platform pages
    pages/modplatform/VanillaPage.ui
    pages/modplatform/atlauncher/AtlPage.ui
    pages/modplatform/ftb/FtbPage.ui
    pages/modplatform/legacy_ftb/Page.ui
    pages/modplatform/flame/FlamePage.ui
    pages/modplatform/technic/TechnicPage.ui
    pages/modplatform/ImportPage.ui

    # Platform Dialogs
    pages/modplatform/atlauncher/AtlOptionalModDialog.ui

    # Dialogs
    dialogs/CopyInstanceDialog.ui
    dialogs/NewComponentDialog.ui
    dialogs/NewInstanceDialog.ui
    dialogs/AboutDialog.ui
    dialogs/ProgressDialog.ui
    dialogs/IconPickerDialog.ui
    dialogs/ProfileSelectDialog.ui
    dialogs/ProfileSetupDialog.ui
    dialogs/EditAccountDialog.ui
    dialogs/ExportInstanceDialog.ui
    dialogs/LoginDialog.ui
    dialogs/MSALoginDialog.ui
    dialogs/UpdateDialog.ui
    dialogs/NotificationDialog.ui
    dialogs/SkinUploadDialog.ui

    # Widgets/other
    widgets/CustomCommands.ui
    widgets/MCModInfoFrame.ui
)

set(LAUNCHER_QRCS
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
    ../${Launcher_Branding_LogoQRC}
)

######## Windows resource files ########
if(WIN32)
    set(LAUNCHER_RCS ../${Launcher_Branding_WindowsRC})
endif()

# Qt 5 stuff
qt5_wrap_ui(LAUNCHER_UI ${LAUNCHER_UIS})
qt5_add_resources(LAUNCHER_RESOURCES ${LAUNCHER_QRCS})

# Add executable
add_library(Launcher_logic STATIC ${LOGIC_SOURCES} ${LAUNCHER_SOURCES} ${LAUNCHER_UI} ${LAUNCHER_RESOURCES})
target_link_libraries(Launcher_logic
    systeminfo
    Launcher_quazip
    Launcher_classparser
    ${NBT_NAME}
    ${ZLIB_LIBRARIES}
    optional-bare
    tomlc99
    BuildConfig
    Katabasis
)
target_link_libraries(Launcher_logic
    Qt5::Core
    Qt5::Xml
    Qt5::Network
    Qt5::Concurrent
    Qt5::Gui
)
target_link_libraries(Launcher_logic
    Launcher_iconfix
    ${QUAZIP_LIBRARIES}
    hoedown
    Launcher_rainbow
    LocalPeer
    ganalytics
)

target_link_libraries(Launcher_logic secrets)

add_executable(${Launcher_Name} MACOSX_BUNDLE WIN32 main.cpp ${LAUNCHER_RCS})
target_link_libraries(${Launcher_Name} Launcher_logic)

if(DEFINED Launcher_APP_BINARY_NAME)
    set_target_properties(${Launcher_Name} PROPERTIES OUTPUT_NAME "${Launcher_APP_BINARY_NAME}")
endif()
if(DEFINED Launcher_BINARY_RPATH)
    SET_TARGET_PROPERTIES(${Launcher_Name} PROPERTIES INSTALL_RPATH "${Launcher_BINARY_RPATH}")
endif()

if(DEFINED Launcher_APP_BINARY_DEFS)
    target_compile_definitions(${Launcher_Name} PRIVATE ${Launcher_APP_BINARY_DEFS})
    target_compile_definitions(Launcher_logic PRIVATE ${Launcher_APP_BINARY_DEFS})
endif()

install(TARGETS ${Launcher_Name}
    BUNDLE DESTINATION ${BUNDLE_DEST_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
    RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
)

#### The bundle mess! ####
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
        # Style plugins
        if(EXISTS "${QT_PLUGINS_DIR}/styles")
            install(
                DIRECTORY "${QT_PLUGINS_DIR}/styles"
                DESTINATION ${PLUGIN_DEST_DIR}
                COMPONENT Runtime
            )
        endif()
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
        # Style plugins
        if(EXISTS "${QT_PLUGINS_DIR}/styles")
            install(
                DIRECTORY "${QT_PLUGINS_DIR}/styles"
                DESTINATION ${PLUGIN_DEST_DIR}
                COMPONENT Runtime
                REGEX "d\\." EXCLUDE
                REGEX "_debug\\." EXCLUDE
                REGEX "\\.dSYM" EXCLUDE
            )
        endif()
    endif()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/install_prereqs.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake" COMPONENT Runtime)
endif()
