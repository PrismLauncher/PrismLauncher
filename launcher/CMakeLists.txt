project(application)

################################ FILES ################################

######## Sources and headers ########

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

ecm_add_test(FileSystem_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME FileSystem)  # TODO: needs testdata

ecm_add_test(GZip_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME GZip)

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
    net/NetUtils.h
    net/PasteUpload.cpp
    net/PasteUpload.h
    net/Sink.h
    net/Validator.h
    net/Upload.cpp
    net/Upload.h
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
    launch/steps/QuitAfterGameStop.cpp
    launch/steps/QuitAfterGameStop.h
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
    updater/ExternalUpdater.h
)

set(MAC_UPDATE_SOURCES
    updater/MacSparkleUpdater.h
    updater/MacSparkleUpdater.mm
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
    minecraft/auth/steps/MinecraftProfileStepMojang.cpp
    minecraft/auth/steps/MinecraftProfileStepMojang.h
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

    minecraft/mod/MetadataHandler.h
    minecraft/mod/Mod.h
    minecraft/mod/Mod.cpp
    minecraft/mod/ModDetails.h
    minecraft/mod/ModFolderModel.h
    minecraft/mod/ModFolderModel.cpp
    minecraft/mod/Resource.h
    minecraft/mod/Resource.cpp
    minecraft/mod/ResourceFolderModel.h
    minecraft/mod/ResourceFolderModel.cpp
    minecraft/mod/ResourcePack.h
    minecraft/mod/ResourcePack.cpp
    minecraft/mod/ResourcePackFolderModel.h
    minecraft/mod/ResourcePackFolderModel.cpp
    minecraft/mod/TexturePackFolderModel.h
    minecraft/mod/TexturePackFolderModel.cpp
    minecraft/mod/ShaderPackFolderModel.h
    minecraft/mod/tasks/BasicFolderLoadTask.h
    minecraft/mod/tasks/ModFolderLoadTask.h
    minecraft/mod/tasks/ModFolderLoadTask.cpp
    minecraft/mod/tasks/LocalModParseTask.h
    minecraft/mod/tasks/LocalModParseTask.cpp
    minecraft/mod/tasks/LocalModUpdateTask.h
    minecraft/mod/tasks/LocalModUpdateTask.cpp
    minecraft/mod/tasks/LocalResourcePackParseTask.h
    minecraft/mod/tasks/LocalResourcePackParseTask.cpp

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
    minecraft/Agent.h)

ecm_add_test(minecraft/GradleSpecifier_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME GradleSpecifier)

if(BUILD_TESTING)
    add_executable(PackageManifest
        mojang/PackageManifest_test.cpp
    )
    target_link_libraries(PackageManifest
        Launcher_logic
        Qt${QT_VERSION_MAJOR}::Test
    )
    target_include_directories(PackageManifest
        PRIVATE ../cmake/UnitTest/
    )
    add_test(
        NAME PackageManifest
        COMMAND PackageManifest
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# TODO: needs minecraft/testdata
ecm_add_test(minecraft/MojangVersionFormat_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME MojangVersionFormat)

ecm_add_test(minecraft/Library_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Library)

# FIXME: shares data with FileSystem test
# TODO: needs testdata
ecm_add_test(minecraft/mod/ResourceFolderModel_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ResourceFolderModel)

ecm_add_test(minecraft/mod/ResourcePackParse_test.cpp
    LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ResourcePackParse)

ecm_add_test(minecraft/ParseUtils_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ParseUtils)

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
    tasks/ConcurrentTask.h
    tasks/ConcurrentTask.cpp
    tasks/SequentialTask.h
    tasks/SequentialTask.cpp
    tasks/MultipleOptionsTask.h
    tasks/MultipleOptionsTask.cpp
)

ecm_add_test(tasks/Task_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Task)

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

ecm_add_test(settings/INIFile_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME INIFile)

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

ecm_add_test(java/JavaVersion_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME JavaVersion)

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

set(API_SOURCES
    modplatform/ModIndex.h
    modplatform/ModIndex.cpp

    modplatform/ModAPI.h

    modplatform/EnsureMetadataTask.h
    modplatform/EnsureMetadataTask.cpp

    modplatform/CheckUpdateTask.h

    modplatform/flame/FlameAPI.h
    modplatform/flame/FlameAPI.cpp
    modplatform/modrinth/ModrinthAPI.h
    modplatform/modrinth/ModrinthAPI.cpp
    modplatform/helpers/NetworkModAPI.h
    modplatform/helpers/NetworkModAPI.cpp
    modplatform/helpers/HashUtils.h
    modplatform/helpers/HashUtils.cpp
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
    modplatform/flame/FlameCheckUpdate.cpp
    modplatform/flame/FlameCheckUpdate.h
)

set(MODRINTH_SOURCES
    modplatform/modrinth/ModrinthPackIndex.cpp
    modplatform/modrinth/ModrinthPackIndex.h
    modplatform/modrinth/ModrinthPackManifest.cpp
    modplatform/modrinth/ModrinthPackManifest.h
    modplatform/modrinth/ModrinthCheckUpdate.cpp
    modplatform/modrinth/ModrinthCheckUpdate.h
)

set(MODPACKSCH_SOURCES
    modplatform/modpacksch/FTBPackInstallTask.h
    modplatform/modpacksch/FTBPackInstallTask.cpp
    modplatform/modpacksch/FTBPackManifest.h
    modplatform/modpacksch/FTBPackManifest.cpp
)

set(PACKWIZ_SOURCES
    modplatform/packwiz/Packwiz.h
    modplatform/packwiz/Packwiz.cpp
)

# TODO: needs modplatform/packwiz/testdata
ecm_add_test(modplatform/packwiz/Packwiz_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Packwiz)

set(TECHNIC_SOURCES
    modplatform/technic/SingleZipPackInstallTask.h
    modplatform/technic/SingleZipPackInstallTask.cpp
    modplatform/technic/SolderPackInstallTask.h
    modplatform/technic/SolderPackInstallTask.cpp
    modplatform/technic/SolderPackManifest.h
    modplatform/technic/SolderPackManifest.cpp
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
    modplatform/atlauncher/ATLShareCode.cpp
    modplatform/atlauncher/ATLShareCode.h
)

ecm_add_test(meta/Index_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Index)

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
    ${API_SOURCES}
    ${FTB_SOURCES}
    ${FLAME_SOURCES}
    ${MODRINTH_SOURCES}
    ${MODPACKSCH_SOURCES}
    ${PACKWIZ_SOURCES}
    ${TECHNIC_SOURCES}
    ${ATLAUNCHER_SOURCES}
)

if(APPLE)
    set (LOGIC_SOURCES ${LOGIC_SOURCES} ${MAC_UPDATE_SOURCES})
endif()

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
    ui/setupwizard/PasteWizardPage.cpp
    ui/setupwizard/PasteWizardPage.h

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
    ui/pages/instance/ExternalResourcesPage.cpp
    ui/pages/instance/ExternalResourcesPage.h
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

    ui/pages/modplatform/ModPage.cpp
    ui/pages/modplatform/ModPage.h
    ui/pages/modplatform/ModModel.cpp
    ui/pages/modplatform/ModModel.h

    ui/pages/modplatform/atlauncher/AtlFilterModel.cpp
    ui/pages/modplatform/atlauncher/AtlFilterModel.h
    ui/pages/modplatform/atlauncher/AtlListModel.cpp
    ui/pages/modplatform/atlauncher/AtlListModel.h
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.cpp
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.h
    ui/pages/modplatform/atlauncher/AtlPage.cpp
    ui/pages/modplatform/atlauncher/AtlPage.h
    ui/pages/modplatform/atlauncher/AtlUserInteractionSupportImpl.cpp
    ui/pages/modplatform/atlauncher/AtlUserInteractionSupportImpl.h

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

    ui/pages/modplatform/modrinth/ModrinthPage.cpp
    ui/pages/modplatform/modrinth/ModrinthPage.h
    ui/pages/modplatform/modrinth/ModrinthModel.cpp
    ui/pages/modplatform/modrinth/ModrinthModel.h

    ui/pages/modplatform/technic/TechnicModel.cpp
    ui/pages/modplatform/technic/TechnicModel.h
    ui/pages/modplatform/technic/TechnicPage.cpp
    ui/pages/modplatform/technic/TechnicPage.h

    ui/pages/modplatform/ImportPage.cpp
    ui/pages/modplatform/ImportPage.h

    ui/pages/modplatform/modrinth/ModrinthModModel.cpp
    ui/pages/modplatform/modrinth/ModrinthModModel.h
    ui/pages/modplatform/modrinth/ModrinthModPage.cpp
    ui/pages/modplatform/modrinth/ModrinthModPage.h

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
    ui/dialogs/NewsDialog.cpp
    ui/dialogs/NewsDialog.h
    ui/pagedialog/PageDialog.cpp
    ui/pagedialog/PageDialog.h
    ui/dialogs/ProgressDialog.cpp
    ui/dialogs/ProgressDialog.h
    ui/dialogs/ReviewMessageBox.cpp
    ui/dialogs/ReviewMessageBox.h
    ui/dialogs/UpdateDialog.cpp
    ui/dialogs/UpdateDialog.h
    ui/dialogs/VersionSelectDialog.cpp
    ui/dialogs/VersionSelectDialog.h
    ui/dialogs/SkinUploadDialog.cpp
    ui/dialogs/SkinUploadDialog.h
    ui/dialogs/ModDownloadDialog.cpp
    ui/dialogs/ModDownloadDialog.h
    ui/dialogs/ScrollMessageBox.cpp
    ui/dialogs/ScrollMessageBox.h
    ui/dialogs/BlockedModsDialog.cpp
    ui/dialogs/BlockedModsDialog.h
    ui/dialogs/ChooseProviderDialog.h
    ui/dialogs/ChooseProviderDialog.cpp
    ui/dialogs/ModUpdateDialog.cpp
    ui/dialogs/ModUpdateDialog.h

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
    ui/widgets/InfoFrame.cpp
    ui/widgets/InfoFrame.h
    ui/widgets/ModFilterWidget.cpp
    ui/widgets/ModFilterWidget.h
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

qt_wrap_ui(LAUNCHER_UI
    ui/setupwizard/PasteWizardPage.ui
    ui/pages/global/AccountListPage.ui
    ui/pages/global/JavaPage.ui
    ui/pages/global/LauncherPage.ui
    ui/pages/global/APIPage.ui
    ui/pages/global/ProxyPage.ui
    ui/pages/global/MinecraftPage.ui
    ui/pages/global/ExternalToolsPage.ui
    ui/pages/instance/ExternalResourcesPage.ui
    ui/pages/instance/NotesPage.ui
    ui/pages/instance/LogPage.ui
    ui/pages/instance/ServersPage.ui
    ui/pages/instance/GameOptionsPage.ui
    ui/pages/instance/OtherLogsPage.ui
    ui/pages/instance/InstanceSettingsPage.ui
    ui/pages/instance/VersionPage.ui
    ui/pages/instance/WorldListPage.ui
    ui/pages/instance/ScreenshotsPage.ui
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.ui
    ui/pages/modplatform/atlauncher/AtlPage.ui
    ui/pages/modplatform/VanillaPage.ui
    ui/pages/modplatform/ModPage.ui
    ui/pages/modplatform/flame/FlamePage.ui
    ui/pages/modplatform/legacy_ftb/Page.ui
    ui/pages/modplatform/ImportPage.ui
    ui/pages/modplatform/ftb/FtbPage.ui
    ui/pages/modplatform/modrinth/ModrinthPage.ui
    ui/pages/modplatform/technic/TechnicPage.ui
    ui/widgets/InstanceCardWidget.ui
    ui/widgets/CustomCommands.ui
    ui/widgets/InfoFrame.ui
    ui/widgets/ModFilterWidget.ui
    ui/dialogs/CopyInstanceDialog.ui
    ui/dialogs/ProfileSetupDialog.ui
    ui/dialogs/ProgressDialog.ui
    ui/dialogs/NewInstanceDialog.ui
    ui/dialogs/UpdateDialog.ui
    ui/dialogs/NewComponentDialog.ui
    ui/dialogs/NewsDialog.ui
    ui/dialogs/ProfileSelectDialog.ui
    ui/dialogs/SkinUploadDialog.ui
    ui/dialogs/ExportInstanceDialog.ui
    ui/dialogs/IconPickerDialog.ui
    ui/dialogs/MSALoginDialog.ui
    ui/dialogs/OfflineLoginDialog.ui
    ui/dialogs/AboutDialog.ui
    ui/dialogs/LoginDialog.ui
    ui/dialogs/EditAccountDialog.ui
    ui/dialogs/ReviewMessageBox.ui
    ui/dialogs/ScrollMessageBox.ui
    ui/dialogs/BlockedModsDialog.ui
    ui/dialogs/ChooseProviderDialog.ui
)

qt_add_resources(LAUNCHER_RESOURCES
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
    set(LAUNCHER_RCS ${CMAKE_CURRENT_BINARY_DIR}/../${Launcher_Branding_WindowsRC})
endif()

# Add executable
add_library(Launcher_logic STATIC ${LOGIC_SOURCES} ${LAUNCHER_SOURCES} ${LAUNCHER_UI} ${LAUNCHER_RESOURCES})
target_link_libraries(Launcher_logic
    systeminfo
    Launcher_classparser
    Launcher_murmur2
    nbt++
    ${ZLIB_LIBRARIES}
    tomlc99
    BuildConfig
    Katabasis
    Qt${QT_VERSION_MAJOR}::Widgets
)

if (UNIX AND NOT CYGWIN AND NOT APPLE)
    target_link_libraries(Launcher_logic
        gamemode
    )
endif()

target_link_libraries(Launcher_logic
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    ${Launcher_QT_LIBS}
)
target_link_libraries(Launcher_logic
    QuaZip::QuaZip
    hoedown
    LocalPeer
    Launcher_rainbow
)
if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@loader_path/../Frameworks/")

    file(DOWNLOAD ${MACOSX_SPARKLE_DOWNLOAD_URL} ${CMAKE_BINARY_DIR}/Sparkle.tar.xz EXPECTED_HASH SHA256=${MACOSX_SPARKLE_SHA256})
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_BINARY_DIR}/Sparkle.tar.xz DESTINATION ${CMAKE_BINARY_DIR}/frameworks/Sparkle)

    find_library(SPARKLE_FRAMEWORK Sparkle "${CMAKE_BINARY_DIR}/frameworks/Sparkle")
    target_link_libraries(Launcher_logic
        "-framework AppKit"
        "-framework Carbon"
        "-framework Foundation"
        "-framework ApplicationServices"
    )
    target_link_libraries(Launcher_logic ${SPARKLE_FRAMEWORK})
endif()

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
    BUNDLE DESTINATION "." COMPONENT Runtime
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
    RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
    FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR} COMPONENT Runtime
)

if (UNIX AND APPLE)
    # Add Sparkle updater
    # It has to be copied here instead of just allowing fixup_bundle to install it, otherwise essential parts of
    # the framework aren't installed
    install(DIRECTORY ${MACOSX_SPARKLE_DIR}/Sparkle.framework DESTINATION ${FRAMEWORK_DEST_DIR} USE_SOURCE_PERMISSIONS)
endif()

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
        # Style plugins
        if(EXISTS "${QT_PLUGINS_DIR}/styles")
            install(
                DIRECTORY "${QT_PLUGINS_DIR}/styles"
                DESTINATION ${PLUGIN_DEST_DIR}
                COMPONENT Runtime
            )
        endif()
        # TLS plugins (Qt 6 only)
        if(EXISTS "${QT_PLUGINS_DIR}/tls")
            install(
                DIRECTORY "${QT_PLUGINS_DIR}/tls"
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
            REGEX "tga|tiff|mng" EXCLUDE
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
        # TLS plugins (Qt 6 only)
        if(EXISTS "${QT_PLUGINS_DIR}/tls")
            install(
                DIRECTORY "${QT_PLUGINS_DIR}/tls"
                DESTINATION ${PLUGIN_DEST_DIR}
                COMPONENT Runtime
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
