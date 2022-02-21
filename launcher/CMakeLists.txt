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

    # Mod downloading task
    ModDownloadTask.h
    ModDownloadTask.cpp

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
    launch/steps/CheckJava.cpp
    launch/steps/CheckJava.h
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
    minecraft/auth/AccountData.cpp
    minecraft/auth/AccountData.h
    minecraft/auth/AccountList.cpp
    minecraft/auth/AccountList.h
    minecraft/auth/AccountTask.cpp
    minecraft/auth/AccountTask.h
    minecraft/auth/AuthRequest.cpp
    minecraft/auth/AuthRequest.h
    minecraft/auth/AuthSession.cpp
    minecraft/auth/AuthSession.h
    minecraft/auth/AuthStep.cpp
    minecraft/auth/AuthStep.h
    minecraft/auth/MinecraftAccount.cpp
    minecraft/auth/MinecraftAccount.h
    minecraft/auth/Parsers.cpp
    minecraft/auth/Parsers.h
    minecraft/auth/Yggdrasil.cpp
    minecraft/auth/Yggdrasil.h

    minecraft/auth/flows/AuthFlow.cpp
    minecraft/auth/flows/AuthFlow.h
    minecraft/auth/flows/Mojang.cpp
    minecraft/auth/flows/Mojang.h
    minecraft/auth/flows/MSA.cpp
    minecraft/auth/flows/MSA.h
    minecraft/auth/flows/Offline.cpp
    minecraft/auth/flows/Offline.h

    minecraft/auth/steps/OfflineStep.cpp
    minecraft/auth/steps/OfflineStep.h
    minecraft/auth/steps/EntitlementsStep.cpp
    minecraft/auth/steps/EntitlementsStep.h
    minecraft/auth/steps/GetSkinStep.cpp
    minecraft/auth/steps/GetSkinStep.h
    minecraft/auth/steps/LauncherLoginStep.cpp
    minecraft/auth/steps/LauncherLoginStep.h
    minecraft/auth/steps/MigrationEligibilityStep.cpp
    minecraft/auth/steps/MigrationEligibilityStep.h
    minecraft/auth/steps/MinecraftProfileStep.cpp
    minecraft/auth/steps/MinecraftProfileStep.h
    minecraft/auth/steps/MSAStep.cpp
    minecraft/auth/steps/MSAStep.h
    minecraft/auth/steps/XboxAuthorizationStep.cpp
    minecraft/auth/steps/XboxAuthorizationStep.h
    minecraft/auth/steps/XboxProfileStep.cpp
    minecraft/auth/steps/XboxProfileStep.h
    minecraft/auth/steps/XboxUserStep.cpp
    minecraft/auth/steps/XboxUserStep.h
    minecraft/auth/steps/YggdrasilStep.cpp
    minecraft/auth/steps/YggdrasilStep.h

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
    modplatform/flame/FlameModIndex.cpp
    modplatform/flame/FlameModIndex.h
    modplatform/flame/PackManifest.h
    modplatform/flame/PackManifest.cpp
    modplatform/flame/FileResolvingTask.h
    modplatform/flame/FileResolvingTask.cpp
)

set(MODRINTH_SOURCES
    modplatform/modrinth/ModrinthPackIndex.cpp
    modplatform/modrinth/ModrinthPackIndex.h
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
    ${MODRINTH_SOURCES}
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
    VersionProxyModel.h
    VersionProxyModel.cpp
    HoeDown.h

    # Super secret!
    KonamiCode.h
    KonamiCode.cpp

    # Bundled resources
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

    # Icons
    icons/MMCIcon.h
    icons/MMCIcon.cpp
    icons/IconList.h
    icons/IconList.cpp

    # GUI - windows
    ui/GuiUtil.h
    ui/GuiUtil.cpp
    ui/ColorCache.h
    ui/ColorCache.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
    ui/InstanceWindow.h
    ui/InstanceWindow.cpp

    # FIXME: maybe find a better home for this.
    SkinUtils.cpp
    SkinUtils.h

    # GUI - setup wizard
    ui/setupwizard/SetupWizard.h
    ui/setupwizard/SetupWizard.cpp
    ui/setupwizard/BaseWizardPage.h
    ui/setupwizard/JavaWizardPage.cpp
    ui/setupwizard/JavaWizardPage.h
    ui/setupwizard/LanguageWizardPage.cpp
    ui/setupwizard/LanguageWizardPage.h

    # GUI - themes
    ui/themes/FusionTheme.cpp
    ui/themes/FusionTheme.h
    ui/themes/BrightTheme.cpp
    ui/themes/BrightTheme.h
    ui/themes/CustomTheme.cpp
    ui/themes/CustomTheme.h
    ui/themes/DarkTheme.cpp
    ui/themes/DarkTheme.h
    ui/themes/ITheme.cpp
    ui/themes/ITheme.h
    ui/themes/SystemTheme.cpp
    ui/themes/SystemTheme.h

    # Processes
    LaunchController.h
    LaunchController.cpp

    # page provider for instances
    InstancePageProvider.h

    # Common java checking UI
    JavaCommon.h
    JavaCommon.cpp

    # GUI - paged dialog base
    ui/pages/BasePage.h
    ui/pages/BasePageContainer.h
    ui/pages/BasePageProvider.h

    # GUI - instance pages
    ui/pages/instance/GameOptionsPage.cpp
    ui/pages/instance/GameOptionsPage.h
    ui/pages/instance/VersionPage.cpp
    ui/pages/instance/VersionPage.h
    ui/pages/instance/TexturePackPage.h
    ui/pages/instance/ResourcePackPage.h
    ui/pages/instance/ShaderPackPage.h
    ui/pages/instance/ModFolderPage.cpp
    ui/pages/instance/ModFolderPage.h
    ui/pages/instance/NotesPage.cpp
    ui/pages/instance/NotesPage.h
    ui/pages/instance/LogPage.cpp
    ui/pages/instance/LogPage.h
    ui/pages/instance/InstanceSettingsPage.cpp
    ui/pages/instance/InstanceSettingsPage.h
    ui/pages/instance/ScreenshotsPage.cpp
    ui/pages/instance/ScreenshotsPage.h
    ui/pages/instance/OtherLogsPage.cpp
    ui/pages/instance/OtherLogsPage.h
    ui/pages/instance/ServersPage.cpp
    ui/pages/instance/ServersPage.h
    ui/pages/instance/LegacyUpgradePage.cpp
    ui/pages/instance/LegacyUpgradePage.h
    ui/pages/instance/WorldListPage.cpp
    ui/pages/instance/WorldListPage.h

    # GUI - global settings pages
    ui/pages/global/AccountListPage.cpp
    ui/pages/global/AccountListPage.h
    ui/pages/global/CustomCommandsPage.cpp
    ui/pages/global/CustomCommandsPage.h
    ui/pages/global/ExternalToolsPage.cpp
    ui/pages/global/ExternalToolsPage.h
    ui/pages/global/JavaPage.cpp
    ui/pages/global/JavaPage.h
    ui/pages/global/LanguagePage.cpp
    ui/pages/global/LanguagePage.h
    ui/pages/global/MinecraftPage.cpp
    ui/pages/global/MinecraftPage.h
    ui/pages/global/LauncherPage.cpp
    ui/pages/global/LauncherPage.h
    ui/pages/global/ProxyPage.cpp
    ui/pages/global/ProxyPage.h
    ui/pages/global/APIPage.cpp
    ui/pages/global/APIPage.h

    # GUI - platform pages
    ui/pages/modplatform/VanillaPage.cpp
    ui/pages/modplatform/VanillaPage.h

    ui/pages/modplatform/atlauncher/AtlFilterModel.cpp
    ui/pages/modplatform/atlauncher/AtlFilterModel.h
    ui/pages/modplatform/atlauncher/AtlListModel.cpp
    ui/pages/modplatform/atlauncher/AtlListModel.h
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.cpp
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.h
    ui/pages/modplatform/atlauncher/AtlPage.cpp
    ui/pages/modplatform/atlauncher/AtlPage.h

    ui/pages/modplatform/ftb/FtbFilterModel.cpp
    ui/pages/modplatform/ftb/FtbFilterModel.h
    ui/pages/modplatform/ftb/FtbListModel.cpp
    ui/pages/modplatform/ftb/FtbListModel.h
    ui/pages/modplatform/ftb/FtbPage.cpp
    ui/pages/modplatform/ftb/FtbPage.h

    ui/pages/modplatform/legacy_ftb/Page.cpp
    ui/pages/modplatform/legacy_ftb/Page.h
    ui/pages/modplatform/legacy_ftb/ListModel.h
    ui/pages/modplatform/legacy_ftb/ListModel.cpp

    ui/pages/modplatform/flame/FlameModel.cpp
    ui/pages/modplatform/flame/FlameModel.h
    ui/pages/modplatform/flame/FlamePage.cpp
    ui/pages/modplatform/flame/FlamePage.h
    ui/pages/modplatform/flame/FlameModModel.cpp
    ui/pages/modplatform/flame/FlameModModel.h
    ui/pages/modplatform/flame/FlameModPage.cpp
    ui/pages/modplatform/flame/FlameModPage.h

    ui/pages/modplatform/technic/TechnicModel.cpp
    ui/pages/modplatform/technic/TechnicModel.h
    ui/pages/modplatform/technic/TechnicPage.cpp
    ui/pages/modplatform/technic/TechnicPage.h

    ui/pages/modplatform/ImportPage.cpp
    ui/pages/modplatform/ImportPage.h

    ui/pages/modplatform/modrinth/ModrinthModel.cpp
    ui/pages/modplatform/modrinth/ModrinthModel.h
    ui/pages/modplatform/modrinth/ModrinthPage.cpp
    ui/pages/modplatform/modrinth/ModrinthPage.h

    # GUI - dialogs
    ui/dialogs/AboutDialog.cpp
    ui/dialogs/AboutDialog.h
    ui/dialogs/ProfileSelectDialog.cpp
    ui/dialogs/ProfileSelectDialog.h
    ui/dialogs/ProfileSetupDialog.cpp
    ui/dialogs/ProfileSetupDialog.h
    ui/dialogs/CopyInstanceDialog.cpp
    ui/dialogs/CopyInstanceDialog.h
    ui/dialogs/CustomMessageBox.cpp
    ui/dialogs/CustomMessageBox.h
    ui/dialogs/EditAccountDialog.cpp
    ui/dialogs/EditAccountDialog.h
    ui/dialogs/ExportInstanceDialog.cpp
    ui/dialogs/ExportInstanceDialog.h
    ui/dialogs/IconPickerDialog.cpp
    ui/dialogs/IconPickerDialog.h
    ui/dialogs/LoginDialog.cpp
    ui/dialogs/LoginDialog.h
    ui/dialogs/MSALoginDialog.cpp
    ui/dialogs/MSALoginDialog.h
    ui/dialogs/OfflineLoginDialog.cpp
    ui/dialogs/OfflineLoginDialog.h
    ui/dialogs/NewComponentDialog.cpp
    ui/dialogs/NewComponentDialog.h
    ui/dialogs/NewInstanceDialog.cpp
    ui/dialogs/NewInstanceDialog.h
    ui/pagedialog/PageDialog.cpp
    ui/pagedialog/PageDialog.h
    ui/dialogs/ProgressDialog.cpp
    ui/dialogs/ProgressDialog.h
    ui/dialogs/UpdateDialog.cpp
    ui/dialogs/UpdateDialog.h
    ui/dialogs/VersionSelectDialog.cpp
    ui/dialogs/VersionSelectDialog.h
    ui/dialogs/SkinUploadDialog.cpp
    ui/dialogs/SkinUploadDialog.h
    ui/dialogs/ModDownloadDialog.cpp
    ui/dialogs/ModDownloadDialog.h


    # GUI - widgets
    ui/widgets/Common.cpp
    ui/widgets/Common.h
    ui/widgets/CustomCommands.cpp
    ui/widgets/CustomCommands.h
    ui/widgets/DropLabel.cpp
    ui/widgets/DropLabel.h
    ui/widgets/FocusLineEdit.cpp
    ui/widgets/FocusLineEdit.h
    ui/widgets/IconLabel.cpp
    ui/widgets/IconLabel.h
    ui/widgets/JavaSettingsWidget.cpp
    ui/widgets/JavaSettingsWidget.h
    ui/widgets/LabeledToolButton.cpp
    ui/widgets/LabeledToolButton.h
    ui/widgets/LanguageSelectionWidget.cpp
    ui/widgets/LanguageSelectionWidget.h
    ui/widgets/LineSeparator.cpp
    ui/widgets/LineSeparator.h
    ui/widgets/LogView.cpp
    ui/widgets/LogView.h
    ui/widgets/MCModInfoFrame.cpp
    ui/widgets/MCModInfoFrame.h
    ui/widgets/ModListView.cpp
    ui/widgets/ModListView.h
    ui/widgets/PageContainer.cpp
    ui/widgets/PageContainer.h
    ui/widgets/PageContainer_p.h
    ui/widgets/VersionListView.cpp
    ui/widgets/VersionListView.h
    ui/widgets/VersionSelectWidget.cpp
    ui/widgets/VersionSelectWidget.h
    ui/widgets/ProgressWidget.h
    ui/widgets/ProgressWidget.cpp
    ui/widgets/WideBar.h
    ui/widgets/WideBar.cpp

    # GUI - instance group view
    ui/instanceview/InstanceProxyModel.cpp
    ui/instanceview/InstanceProxyModel.h
    ui/instanceview/AccessibleInstanceView.cpp
    ui/instanceview/AccessibleInstanceView.h
    ui/instanceview/AccessibleInstanceView_p.h
    ui/instanceview/InstanceView.cpp
    ui/instanceview/InstanceView.h
    ui/instanceview/InstanceDelegate.cpp
    ui/instanceview/InstanceDelegate.h
    ui/instanceview/VisualGroup.cpp
    ui/instanceview/VisualGroup.h
)

qt5_wrap_ui(LAUNCHER_UI
    ui/pages/global/AccountListPage.ui
    ui/pages/global/JavaPage.ui
    ui/pages/global/LauncherPage.ui
    ui/pages/global/APIPage.ui
    ui/pages/global/ProxyPage.ui
    ui/pages/global/MinecraftPage.ui
    ui/pages/global/ExternalToolsPage.ui
    ui/pages/instance/ModFolderPage.ui
    ui/pages/instance/NotesPage.ui
    ui/pages/instance/LogPage.ui
    ui/pages/instance/ServersPage.ui
    ui/pages/instance/GameOptionsPage.ui
    ui/pages/instance/OtherLogsPage.ui
    ui/pages/instance/InstanceSettingsPage.ui
    ui/pages/instance/VersionPage.ui
    ui/pages/instance/WorldListPage.ui
    ui/pages/instance/LegacyUpgradePage.ui
    ui/pages/instance/ScreenshotsPage.ui
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.ui
    ui/pages/modplatform/atlauncher/AtlPage.ui
    ui/pages/modplatform/VanillaPage.ui
    ui/pages/modplatform/flame/FlamePage.ui
    ui/pages/modplatform/flame/FlameModPage.ui
    ui/pages/modplatform/legacy_ftb/Page.ui
    ui/pages/modplatform/ImportPage.ui
    ui/pages/modplatform/ftb/FtbPage.ui
    ui/pages/modplatform/technic/TechnicPage.ui
    ui/pages/modplatform/modrinth/ModrinthPage.ui
    ui/widgets/InstanceCardWidget.ui
    ui/widgets/CustomCommands.ui
    ui/widgets/MCModInfoFrame.ui
    ui/dialogs/CopyInstanceDialog.ui
    ui/dialogs/ProfileSetupDialog.ui
    ui/dialogs/ProgressDialog.ui
    ui/dialogs/NewInstanceDialog.ui
    ui/dialogs/UpdateDialog.ui
    ui/dialogs/NewComponentDialog.ui
    ui/dialogs/ProfileSelectDialog.ui
    ui/dialogs/SkinUploadDialog.ui
    ui/dialogs/ExportInstanceDialog.ui
    ui/dialogs/IconPickerDialog.ui
    ui/dialogs/MSALoginDialog.ui
    ui/dialogs/OfflineLoginDialog.ui
    ui/dialogs/AboutDialog.ui
    ui/dialogs/LoginDialog.ui
    ui/dialogs/EditAccountDialog.ui
)

qt5_add_resources(LAUNCHER_RESOURCES
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

# Add executable
add_library(Launcher_logic STATIC ${LOGIC_SOURCES} ${LAUNCHER_SOURCES} ${LAUNCHER_UI} ${LAUNCHER_RESOURCES})
target_link_libraries(Launcher_logic
    systeminfo
    Launcher_classparser
    nbt++
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
    QuaZip::QuaZip
    hoedown
    PolyMC_rainbow
    LocalPeer
)

target_link_libraries(Launcher_logic)

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
