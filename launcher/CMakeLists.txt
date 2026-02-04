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
    InstanceDirUpdate.h
    InstanceDirUpdate.cpp
    NullInstance.h
    MMCZip.h
    MMCZip.cpp
    archive/ArchiveReader.cpp
    archive/ArchiveReader.h
    archive/ArchiveWriter.cpp
    archive/ArchiveWriter.h
    archive/ExportToZipTask.cpp
    archive/ExportToZipTask.h
    archive/ExtractZipTask.cpp
    archive/ExtractZipTask.h
    StringUtils.h
    StringUtils.cpp
    QVariantUtils.h
    RuntimeContext.h
    PSaveFile.h

    # Basic instance manipulation tasks (derived from InstanceTask)
    InstanceCreationTask.h
    InstanceCreationTask.cpp
    InstanceCopyPrefs.h
    InstanceCopyPrefs.cpp
    InstanceCopyTask.h
    InstanceCopyTask.cpp
    InstanceImportTask.h
    InstanceImportTask.cpp

    # Resource downloading task
    ResourceDownloadTask.h
    ResourceDownloadTask.cpp

    # Use tracking separate from memory management
    Usable.h

    # Prefix tree where node names are strings between separators
    SeparatorPrefixTree.h

    # String filters
    Filter.h

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

    MTPixmapCache.h

    # Assertion helper
    AssertHelpers.h
)
if (UNIX AND NOT CYGWIN AND NOT APPLE)
    set(CORE_SOURCES
    ${CORE_SOURCES}

    # MangoHud
    MangoHud.h
    MangoHud.cpp
    )
endif()

set(NET_SOURCES
    # network stuffs
    net/ByteArraySink.h
    net/ChecksumValidator.h
    net/Download.cpp
    net/Download.h
    net/DummySink.h
    net/FileSink.cpp
    net/FileSink.h
    net/HttpMetaCache.cpp
    net/HttpMetaCache.h
    net/MetaCacheSink.cpp
    net/MetaCacheSink.h
    net/Logging.h
    net/Logging.cpp
    net/NetJob.cpp
    net/NetJob.h
    net/NetUtils.h
    net/PasteUpload.cpp
    net/PasteUpload.h
    net/Sink.h
    net/Validator.h
    net/Upload.cpp
    net/Upload.h
    net/HeaderProxy.h
    net/RawHeaderProxy.h
    net/ApiHeaderProxy.h
    net/ApiDownload.h
    net/ApiDownload.cpp
    net/ApiUpload.cpp
    net/ApiUpload.h
    net/NetRequest.cpp
    net/NetRequest.h
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
    launch/steps/QuitAfterGameStop.cpp
    launch/steps/QuitAfterGameStop.h
    launch/steps/PrintServers.cpp
    launch/steps/PrintServers.h
    launch/LaunchStep.cpp
    launch/LaunchStep.h
    launch/LaunchTask.cpp
    launch/LaunchTask.h
    launch/LogModel.cpp
    launch/LogModel.h
    launch/TaskStepWrapper.cpp
    launch/TaskStepWrapper.h
    logs/LogParser.cpp
    logs/LogParser.h
)

# Old update system
set(UPDATE_SOURCES
    updater/ExternalUpdater.h
)

set(MAC_UPDATE_SOURCES
    updater/MacSparkleUpdater.h
    updater/MacSparkleUpdater.mm
)

set(PRISM_UPDATE_SOURCES
    updater/PrismExternalUpdater.h
    updater/PrismExternalUpdater.cpp
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

    # Logging
    minecraft/Logging.h
    minecraft/Logging.cpp

    # Minecraft support
    minecraft/auth/AccountData.cpp
    minecraft/auth/AccountData.h
    minecraft/auth/AccountList.cpp
    minecraft/auth/AccountList.h
    minecraft/auth/AuthSession.cpp
    minecraft/auth/AuthSession.h
    minecraft/auth/AuthStep.h
    minecraft/auth/MinecraftAccount.cpp
    minecraft/auth/MinecraftAccount.h
    minecraft/auth/Parsers.cpp
    minecraft/auth/Parsers.h

    minecraft/auth/AuthFlow.cpp
    minecraft/auth/AuthFlow.h

    minecraft/auth/steps/EntitlementsStep.cpp
    minecraft/auth/steps/EntitlementsStep.h
    minecraft/auth/steps/GetSkinStep.cpp
    minecraft/auth/steps/GetSkinStep.h
    minecraft/auth/steps/LauncherLoginStep.cpp
    minecraft/auth/steps/LauncherLoginStep.h
    minecraft/auth/steps/MinecraftProfileStep.cpp
    minecraft/auth/steps/MinecraftProfileStep.h
    minecraft/auth/steps/MSADeviceCodeStep.cpp
    minecraft/auth/steps/MSADeviceCodeStep.h
    minecraft/auth/steps/MSAStep.cpp
    minecraft/auth/steps/MSAStep.h
    minecraft/auth/steps/XboxAuthorizationStep.cpp
    minecraft/auth/steps/XboxAuthorizationStep.h
    minecraft/auth/steps/XboxUserStep.cpp
    minecraft/auth/steps/XboxUserStep.h

    minecraft/update/AssetUpdateTask.h
    minecraft/update/AssetUpdateTask.cpp
    minecraft/update/LegacyFMLLibrariesTask.cpp
    minecraft/update/LegacyFMLLibrariesTask.h
    minecraft/update/FoldersTask.cpp
    minecraft/update/FoldersTask.h
    minecraft/update/LibrariesTask.cpp
    minecraft/update/LibrariesTask.h

    minecraft/launch/ClaimAccount.cpp
    minecraft/launch/ClaimAccount.h
    minecraft/launch/CreateGameFolders.cpp
    minecraft/launch/CreateGameFolders.h
    minecraft/launch/EnsureOfflineLibraries.cpp
    minecraft/launch/EnsureOfflineLibraries.h
    minecraft/launch/ModMinecraftJar.cpp
    minecraft/launch/ModMinecraftJar.h
    minecraft/launch/ExtractNatives.cpp
    minecraft/launch/ExtractNatives.h
    minecraft/launch/LauncherPartLaunch.cpp
    minecraft/launch/LauncherPartLaunch.h
    minecraft/launch/MinecraftTarget.cpp
    minecraft/launch/MinecraftTarget.h
    minecraft/launch/PrintInstanceInfo.cpp
    minecraft/launch/PrintInstanceInfo.h
    minecraft/launch/ReconstructAssets.cpp
    minecraft/launch/ReconstructAssets.h
    minecraft/launch/ScanModFolders.cpp
    minecraft/launch/ScanModFolders.h
    minecraft/launch/VerifyJavaInstall.cpp
    minecraft/launch/VerifyJavaInstall.h
    minecraft/launch/AutoInstallJava.cpp
    minecraft/launch/AutoInstallJava.h

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
    minecraft/MojangVersionFormat.cpp
    minecraft/MojangVersionFormat.h
    minecraft/Rule.cpp
    minecraft/Rule.h
    minecraft/OneSixVersionFormat.cpp
    minecraft/OneSixVersionFormat.h
    minecraft/ParseUtils.cpp
    minecraft/ParseUtils.h
    minecraft/ProfileUtils.cpp
    minecraft/ProfileUtils.h
    minecraft/ShortcutUtils.cpp
    minecraft/ShortcutUtils.h
    minecraft/Library.cpp
    minecraft/Library.h
    minecraft/MojangDownloadInfo.h
    minecraft/VanillaInstanceCreationTask.cpp
    minecraft/VanillaInstanceCreationTask.h
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
    minecraft/mod/DataPack.h
    minecraft/mod/DataPack.cpp
    minecraft/mod/DataPackFolderModel.h
    minecraft/mod/DataPackFolderModel.cpp
    minecraft/mod/ResourcePack.h
    minecraft/mod/ResourcePack.cpp
    minecraft/mod/ResourcePackFolderModel.h
    minecraft/mod/ResourcePackFolderModel.cpp
    minecraft/mod/TexturePack.h
    minecraft/mod/TexturePack.cpp
    minecraft/mod/ShaderPack.h
    minecraft/mod/ShaderPack.cpp
    minecraft/mod/WorldSave.h
    minecraft/mod/WorldSave.cpp
    minecraft/mod/TexturePackFolderModel.h
    minecraft/mod/TexturePackFolderModel.cpp
    minecraft/mod/ShaderPackFolderModel.h
    minecraft/mod/ShaderPackFolderModel.cpp
    minecraft/mod/tasks/ResourceFolderLoadTask.h
    minecraft/mod/tasks/ResourceFolderLoadTask.cpp
    minecraft/mod/tasks/LocalModParseTask.h
    minecraft/mod/tasks/LocalModParseTask.cpp
    minecraft/mod/tasks/LocalResourceUpdateTask.h
    minecraft/mod/tasks/LocalResourceUpdateTask.cpp
    minecraft/mod/tasks/LocalDataPackParseTask.h
    minecraft/mod/tasks/LocalDataPackParseTask.cpp
    minecraft/mod/tasks/LocalTexturePackParseTask.h
    minecraft/mod/tasks/LocalTexturePackParseTask.cpp
    minecraft/mod/tasks/LocalShaderPackParseTask.h
    minecraft/mod/tasks/LocalShaderPackParseTask.cpp
    minecraft/mod/tasks/LocalWorldSaveParseTask.h
    minecraft/mod/tasks/LocalWorldSaveParseTask.cpp
    minecraft/mod/tasks/LocalResourceParse.h
    minecraft/mod/tasks/LocalResourceParse.cpp
    minecraft/mod/tasks/GetModDependenciesTask.h
    minecraft/mod/tasks/GetModDependenciesTask.cpp

    # Assets
    minecraft/AssetsUtils.h
    minecraft/AssetsUtils.cpp

    # Minecraft skins
    minecraft/skins/CapeChange.cpp
    minecraft/skins/CapeChange.h
    minecraft/skins/SkinUpload.cpp
    minecraft/skins/SkinUpload.h
    minecraft/skins/SkinDelete.cpp
    minecraft/skins/SkinDelete.h
    minecraft/skins/SkinModel.cpp
    minecraft/skins/SkinModel.h
    minecraft/skins/SkinList.cpp
    minecraft/skins/SkinList.h

    minecraft/Agent.h)

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

set(JAVA_SOURCES
    java/JavaChecker.h
    java/JavaChecker.cpp
    java/JavaInstall.h
    java/JavaInstall.cpp
    java/JavaInstallList.h
    java/JavaInstallList.cpp
    java/JavaUtils.h
    java/JavaUtils.cpp
    java/JavaVersion.h
    java/JavaVersion.cpp

    java/JavaMetadata.h
    java/JavaMetadata.cpp
    java/download/ArchiveDownloadTask.cpp
    java/download/ArchiveDownloadTask.h
    java/download/ManifestDownloadTask.cpp
    java/download/ManifestDownloadTask.h
    java/download/SymlinkTask.cpp
    java/download/SymlinkTask.h

    ui/java/InstallJavaDialog.h
    ui/java/InstallJavaDialog.cpp
    ui/java/VersionList.h
    ui/java/VersionList.cpp
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
    tools/GenericProfiler.cpp
    tools/GenericProfiler.h
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
    modplatform/ResourceType.h
    modplatform/ResourceType.cpp

    modplatform/ResourceAPI.h
    modplatform/ResourceAPI.cpp

    modplatform/EnsureMetadataTask.h
    modplatform/EnsureMetadataTask.cpp

    modplatform/CheckUpdateTask.h

    modplatform/flame/FlameAPI.h
    modplatform/flame/FlameAPI.cpp
    modplatform/modrinth/ModrinthAPI.h
    modplatform/modrinth/ModrinthAPI.cpp
    modplatform/helpers/HashUtils.h
    modplatform/helpers/HashUtils.cpp
    modplatform/helpers/OverrideUtils.h
    modplatform/helpers/OverrideUtils.cpp

    modplatform/helpers/ExportToModList.h
    modplatform/helpers/ExportToModList.cpp
)

set(FTB_SOURCES
    modplatform/legacy_ftb/PackFetchTask.h
    modplatform/legacy_ftb/PackFetchTask.cpp
    modplatform/legacy_ftb/PackInstallTask.h
    modplatform/legacy_ftb/PackInstallTask.cpp
    modplatform/legacy_ftb/PrivatePackManager.h
    modplatform/legacy_ftb/PrivatePackManager.cpp

    modplatform/legacy_ftb/PackHelpers.h

    modplatform/import_ftb/PackInstallTask.h
    modplatform/import_ftb/PackInstallTask.cpp
    modplatform/import_ftb/PackHelpers.h
    modplatform/import_ftb/PackHelpers.cpp

    modplatform/ftb/FTBPackInstallTask.h
    modplatform/ftb/FTBPackInstallTask.cpp
    modplatform/ftb/FTBPackManifest.h
    modplatform/ftb/FTBPackManifest.cpp
)

set(FLAME_SOURCES
    # Flame
    modplatform/flame/FlameModIndex.cpp
    modplatform/flame/FlameModIndex.h
    modplatform/flame/PackManifest.h
    modplatform/flame/PackManifest.cpp
    modplatform/flame/FileResolvingTask.h
    modplatform/flame/FileResolvingTask.cpp
    modplatform/flame/FlameCheckUpdate.cpp
    modplatform/flame/FlameCheckUpdate.h
    modplatform/flame/FlameInstanceCreationTask.h
    modplatform/flame/FlameInstanceCreationTask.cpp
    modplatform/flame/FlamePackExportTask.h
    modplatform/flame/FlamePackExportTask.cpp
)

set(MODRINTH_SOURCES
    modplatform/modrinth/ModrinthPackIndex.cpp
    modplatform/modrinth/ModrinthPackIndex.h
    modplatform/modrinth/ModrinthCheckUpdate.cpp
    modplatform/modrinth/ModrinthCheckUpdate.h
    modplatform/modrinth/ModrinthInstanceCreationTask.cpp
    modplatform/modrinth/ModrinthInstanceCreationTask.h
    modplatform/modrinth/ModrinthPackExportTask.cpp
    modplatform/modrinth/ModrinthPackExportTask.h
)

set(PACKWIZ_SOURCES
    modplatform/packwiz/Packwiz.h
    modplatform/packwiz/Packwiz.cpp
)


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

set(LINKEXE_SOURCES
    console/WindowsConsole.h
    console/WindowsConsole.cpp

    filelink/FileLink.h
    filelink/FileLink.cpp
    FileSystem.h
    FileSystem.cpp
    Exception.h
    StringUtils.h
    StringUtils.cpp
    DesktopServices.h
    DesktopServices.cpp
)

set(PRISMUPDATER_SOURCES
    updater/prismupdater/PrismUpdater.h
    updater/prismupdater/PrismUpdater.cpp
    updater/prismupdater/UpdaterDialogs.h
    updater/prismupdater/UpdaterDialogs.cpp
    updater/prismupdater/GitHubRelease.h
    updater/prismupdater/GitHubRelease.cpp

    Json.h
    Json.cpp
    FileSystem.h
    FileSystem.cpp
    StringUtils.h
    StringUtils.cpp
    DesktopServices.h
    DesktopServices.cpp
    Version.h
    Version.cpp
    Markdown.h
    Markdown.cpp

    # Zip
    MMCZip.h
    MMCZip.cpp
    archive/ArchiveReader.cpp
    archive/ArchiveReader.h
    archive/ArchiveWriter.cpp
    archive/ArchiveWriter.h

    # Time
    MMCTime.h
    MMCTime.cpp

    net/ByteArraySink.h
    net/ChecksumValidator.h
    net/Download.cpp
    net/Download.h
    net/FileSink.cpp
    net/FileSink.h
    net/HttpMetaCache.cpp
    net/HttpMetaCache.h
    net/Logging.h
    net/Logging.cpp
    net/NetRequest.cpp
    net/NetRequest.h
    net/NetJob.cpp
    net/NetJob.h
    net/NetUtils.h
    net/Sink.h
    net/Validator.h
    net/HeaderProxy.h
    net/RawHeaderProxy.h

    ui/dialogs/ProgressDialog.cpp
    ui/dialogs/ProgressDialog.h
    ui/widgets/SubTaskProgressBar.h
    ui/widgets/SubTaskProgressBar.cpp

)

if(WIN32)
    set(PRISMUPDATER_SOURCES
        console/WindowsConsole.h
        console/WindowsConsole.cpp
        ${PRISMUPDATER_SOURCES}
    )
endif()

######## Logging categories ########

ecm_qt_declare_logging_category(CORE_SOURCES
    HEADER Logging.h
    IDENTIFIER authCredentials
    CATEGORY_NAME "launcher.auth.credentials"
    DEFAULT_SEVERITY Warning
    DESCRIPTION "Secrets and credentials for debugging purposes"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER instanceProfileC
    CATEGORY_NAME "launcher.instance.profile"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Profile actions"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER instanceProfileResolveC
    CATEGORY_NAME "launcher.instance.profile.resolve"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Profile component resolusion actions"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER taskLogC
    CATEGORY_NAME "launcher.task"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Task actions"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER taskNetLogC
    CATEGORY_NAME "launcher.task.net"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Task network action"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER taskDownloadLogC
    CATEGORY_NAME "launcher.task.net.download"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Task network download actions"
    EXPORT "${Launcher_Name}"
)
ecm_qt_export_logging_category(
    IDENTIFIER taskUploadLogC
    CATEGORY_NAME "launcher.task.net.upload"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "Task network upload actions"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER taskMetaCacheLogC
    CATEGORY_NAME "launcher.task.net.metacache"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "task network meta-cache actions"
    EXPORT "${Launcher_Name}"
)

ecm_qt_export_logging_category(
    IDENTIFIER taskHttpMetaCacheLogC
    CATEGORY_NAME "launcher.task.net.metacache.http"
    DEFAULT_SEVERITY Debug
    DESCRIPTION "task network http meta-cache actions"
    EXPORT "${Launcher_Name}"
)



if(KDE_INSTALL_LOGGINGCATEGORIESDIR)  # only install if there is a standard path for this
    ecm_qt_install_logging_categories(
        EXPORT "${Launcher_Name}"
        DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
    )
endif()

################################ COMPILE ################################

set(LOGIC_SOURCES
    ${CORE_SOURCES}
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
    ${PACKWIZ_SOURCES}
    ${TECHNIC_SOURCES}
    ${ATLAUNCHER_SOURCES}
)

if(APPLE AND Launcher_ENABLE_UPDATER)
    set (LOGIC_SOURCES ${LOGIC_SOURCES} ${MAC_UPDATE_SOURCES})
else()
    set (LOGIC_SOURCES ${LOGIC_SOURCES} ${PRISM_UPDATE_SOURCES})
endif()

SET(LAUNCHER_SOURCES
    # Application base
    Application.h
    Application.cpp
    DataMigrationTask.h
    DataMigrationTask.cpp
    ApplicationMessage.h
    ApplicationMessage.cpp
    SysInfo.h
    SysInfo.cpp

    # console utils
    console/Console.h

    # GUI - general utilities
    DesktopServices.h
    DesktopServices.cpp
    VersionProxyModel.h
    VersionProxyModel.cpp
    Markdown.h
    Markdown.cpp

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
    resources/breeze_dark/breeze_dark.qrc
    resources/breeze_light/breeze_light.qrc
    resources/OSX/OSX.qrc
    resources/iOS/iOS.qrc
    resources/flat/flat.qrc
    resources/flat_white/flat_white.qrc
    resources/documents/documents.qrc
    resources/shaders/shaders.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/../${Launcher_Branding_LogoQRC}"

    # Icons
    icons/MMCIcon.h
    icons/MMCIcon.cpp
    icons/IconList.h
    icons/IconList.cpp

    # log utils
    logs/AnonymizeLog.cpp
    logs/AnonymizeLog.h

    # GUI - windows
    ui/GuiUtil.h
    ui/GuiUtil.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
    ui/InstanceWindow.h
    ui/InstanceWindow.cpp
    ui/ViewLogWindow.h
    ui/ViewLogWindow.cpp
    ui/ToolTipFilter.h
    ui/ToolTipFilter.cpp

    # FIXME: maybe find a better home for this.
    FileIgnoreProxy.cpp
    FileIgnoreProxy.h
    FastFileIconProvider.cpp
    FastFileIconProvider.h

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
    ui/setupwizard/ThemeWizardPage.h
    ui/setupwizard/AutoJavaWizardPage.cpp
    ui/setupwizard/AutoJavaWizardPage.h
    ui/setupwizard/LoginWizardPage.cpp
    ui/setupwizard/LoginWizardPage.h

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
    ui/themes/HintOverrideProxyStyle.cpp
    ui/themes/HintOverrideProxyStyle.h
    ui/themes/SystemTheme.cpp
    ui/themes/SystemTheme.h
    ui/themes/IconTheme.cpp
    ui/themes/IconTheme.h
    ui/themes/ThemeManager.cpp
    ui/themes/ThemeManager.h
    ui/themes/CatPack.cpp
    ui/themes/CatPack.h
    ui/themes/CatPainter.cpp
    ui/themes/CatPainter.h

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
    ui/pages/instance/VersionPage.cpp
    ui/pages/instance/VersionPage.h
    ui/pages/instance/ManagedPackPage.cpp
    ui/pages/instance/ManagedPackPage.h
    ui/pages/instance/DataPackPage.h
    ui/pages/instance/DataPackPage.cpp
    ui/pages/instance/TexturePackPage.h
    ui/pages/instance/TexturePackPage.cpp
    ui/pages/instance/ResourcePackPage.h
    ui/pages/instance/ResourcePackPage.cpp
    ui/pages/instance/ShaderPackPage.h
    ui/pages/instance/ShaderPackPage.cpp
    ui/pages/instance/ModFolderPage.cpp
    ui/pages/instance/ModFolderPage.h
    ui/pages/instance/NotesPage.cpp
    ui/pages/instance/NotesPage.h
    ui/pages/instance/LogPage.cpp
    ui/pages/instance/LogPage.h
    ui/pages/instance/InstanceSettingsPage.h
    ui/pages/instance/ScreenshotsPage.cpp
    ui/pages/instance/ScreenshotsPage.h
    ui/pages/instance/OtherLogsPage.cpp
    ui/pages/instance/OtherLogsPage.h
    ui/pages/instance/ServersPage.cpp
    ui/pages/instance/ServersPage.h
    ui/pages/instance/WorldListPage.cpp
    ui/pages/instance/WorldListPage.h
    ui/pages/instance/McClient.cpp
    ui/pages/instance/McClient.h
    ui/pages/instance/McResolver.cpp
    ui/pages/instance/McResolver.h
    ui/pages/instance/ServerPingTask.cpp
    ui/pages/instance/ServerPingTask.h

    # GUI - global settings pages
    ui/pages/global/AccountListPage.cpp
    ui/pages/global/AccountListPage.h
    ui/pages/global/ExternalToolsPage.cpp
    ui/pages/global/ExternalToolsPage.h
    ui/pages/global/JavaPage.cpp
    ui/pages/global/JavaPage.h
    ui/pages/global/LanguagePage.cpp
    ui/pages/global/LanguagePage.h
    ui/pages/global/MinecraftPage.h
    ui/pages/global/LauncherPage.cpp
    ui/pages/global/LauncherPage.h
    ui/pages/global/AppearancePage.h
    ui/pages/global/ProxyPage.cpp
    ui/pages/global/ProxyPage.h
    ui/pages/global/APIPage.cpp
    ui/pages/global/APIPage.h

    # GUI - platform pages
    ui/pages/modplatform/CustomPage.cpp
    ui/pages/modplatform/CustomPage.h

    ui/pages/modplatform/ResourcePage.cpp
    ui/pages/modplatform/ResourcePage.h
    ui/pages/modplatform/ResourceModel.cpp
    ui/pages/modplatform/ResourceModel.h

    ui/pages/modplatform/ModPage.cpp
    ui/pages/modplatform/ModPage.h
    ui/pages/modplatform/ModModel.cpp
    ui/pages/modplatform/ModModel.h

    ui/pages/modplatform/ResourcePackPage.cpp
    ui/pages/modplatform/ResourcePackModel.cpp

    # Needed for MOC to find them without a corresponding .cpp
    ui/pages/modplatform/TexturePackPage.h
    ui/pages/modplatform/TexturePackModel.cpp

    ui/pages/modplatform/ShaderPackPage.cpp
    ui/pages/modplatform/ShaderPackModel.cpp

    ui/pages/modplatform/DataPackPage.cpp
    ui/pages/modplatform/DataPackModel.cpp

    ui/pages/modplatform/ModpackProviderBasePage.h

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

    ui/pages/modplatform/import_ftb/ImportFTBPage.cpp
    ui/pages/modplatform/import_ftb/ImportFTBPage.h
    ui/pages/modplatform/import_ftb/ListModel.h
    ui/pages/modplatform/import_ftb/ListModel.cpp

    ui/pages/modplatform/flame/FlameModel.cpp
    ui/pages/modplatform/flame/FlameModel.h
    ui/pages/modplatform/flame/FlamePage.cpp
    ui/pages/modplatform/flame/FlamePage.h
    ui/pages/modplatform/flame/FlameResourceModels.cpp
    ui/pages/modplatform/flame/FlameResourceModels.h
    ui/pages/modplatform/flame/FlameResourcePages.cpp
    ui/pages/modplatform/flame/FlameResourcePages.h

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

    ui/pages/modplatform/OptionalModDialog.cpp
    ui/pages/modplatform/OptionalModDialog.h

    ui/pages/modplatform/modrinth/ModrinthResourcePages.cpp
    ui/pages/modplatform/modrinth/ModrinthResourcePages.h

    # GUI - dialogs
    ui/dialogs/AboutDialog.cpp
    ui/dialogs/AboutDialog.h
    ui/dialogs/ProfileSelectDialog.cpp
    ui/dialogs/ProfileSelectDialog.h
    ui/dialogs/ProfileSetupDialog.cpp
    ui/dialogs/ProfileSetupDialog.h
    ui/dialogs/CopyInstanceDialog.cpp
    ui/dialogs/CopyInstanceDialog.h
    ui/dialogs/CreateShortcutDialog.cpp
    ui/dialogs/CreateShortcutDialog.h
    ui/dialogs/CustomMessageBox.cpp
    ui/dialogs/CustomMessageBox.h
    ui/dialogs/ExportInstanceDialog.cpp
    ui/dialogs/ExportInstanceDialog.h
    ui/dialogs/ExportPackDialog.cpp
    ui/dialogs/ExportPackDialog.h
    ui/dialogs/ExportToModListDialog.cpp
    ui/dialogs/ExportToModListDialog.h
    ui/dialogs/IconPickerDialog.cpp
    ui/dialogs/IconPickerDialog.h
    ui/dialogs/ImportResourceDialog.cpp
    ui/dialogs/ImportResourceDialog.h
    ui/dialogs/MSALoginDialog.cpp
    ui/dialogs/MSALoginDialog.h
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
    ui/dialogs/VersionSelectDialog.cpp
    ui/dialogs/VersionSelectDialog.h
    ui/dialogs/ResourceDownloadDialog.cpp
    ui/dialogs/ResourceDownloadDialog.h
    ui/dialogs/ScrollMessageBox.cpp
    ui/dialogs/ScrollMessageBox.h
    ui/dialogs/BlockedModsDialog.cpp
    ui/dialogs/BlockedModsDialog.h
    ui/dialogs/ChooseProviderDialog.h
    ui/dialogs/ChooseProviderDialog.cpp
    ui/dialogs/ResourceUpdateDialog.cpp
    ui/dialogs/ResourceUpdateDialog.h
    ui/dialogs/InstallLoaderDialog.cpp
    ui/dialogs/InstallLoaderDialog.h
    ui/dialogs/ChooseOfflineNameDialog.cpp
    ui/dialogs/ChooseOfflineNameDialog.h

    ui/dialogs/skins/SkinManageDialog.cpp
    ui/dialogs/skins/SkinManageDialog.h

    ui/dialogs/skins/draw/SkinOpenGLWindow.h
    ui/dialogs/skins/draw/SkinOpenGLWindow.cpp
    ui/dialogs/skins/draw/Scene.h
    ui/dialogs/skins/draw/Scene.cpp
    ui/dialogs/skins/draw/BoxGeometry.h
    ui/dialogs/skins/draw/BoxGeometry.cpp

    # GUI - widgets
    ui/widgets/CheckComboBox.cpp
    ui/widgets/CheckComboBox.h
    ui/widgets/Common.cpp
    ui/widgets/Common.h
    ui/widgets/CustomCommands.cpp
    ui/widgets/CustomCommands.h
    ui/widgets/EnvironmentVariables.cpp
    ui/widgets/EnvironmentVariables.h
    ui/widgets/IconLabel.cpp
    ui/widgets/IconLabel.h
    ui/widgets/JavaWizardWidget.cpp
    ui/widgets/JavaWizardWidget.h
    ui/widgets/LabeledToolButton.cpp
    ui/widgets/LabeledToolButton.h
    ui/widgets/LanguageSelectionWidget.cpp
    ui/widgets/LanguageSelectionWidget.h
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
    ui/widgets/ProjectDescriptionPage.h
    ui/widgets/ProjectDescriptionPage.cpp
    ui/widgets/VariableSizedImageObject.h
    ui/widgets/VariableSizedImageObject.cpp
    ui/widgets/ProjectItem.h
    ui/widgets/ProjectItem.cpp
    ui/widgets/SubTaskProgressBar.h
    ui/widgets/SubTaskProgressBar.cpp
    ui/widgets/VersionListView.cpp
    ui/widgets/VersionListView.h
    ui/widgets/VersionSelectWidget.cpp
    ui/widgets/VersionSelectWidget.h
    ui/widgets/ProgressWidget.h
    ui/widgets/ProgressWidget.cpp
    ui/widgets/WideBar.h
    ui/widgets/WideBar.cpp
    ui/widgets/AppearanceWidget.h
    ui/widgets/AppearanceWidget.cpp
    ui/widgets/MinecraftSettingsWidget.h
    ui/widgets/MinecraftSettingsWidget.cpp
    ui/widgets/JavaSettingsWidget.h
    ui/widgets/JavaSettingsWidget.cpp

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

if (APPLE)
    set(LAUNCHER_SOURCES
        ${LAUNCHER_SOURCES}
        ui/themes/ThemeManager.mm
    )
endif()

if (NOT Apple)
    set(LAUNCHER_SOURCES
    ${LAUNCHER_SOURCES}

    ui/dialogs/UpdateAvailableDialog.h
    ui/dialogs/UpdateAvailableDialog.cpp
)
endif()

if (APPLE)
    set(LAUNCHER_SOURCES
        ${LAUNCHER_SOURCES}

        macsandbox/SecurityBookmarkFileAccess.h
        macsandbox/SecurityBookmarkFileAccess.mm
    )
endif()

if(WIN32)
    set(LAUNCHER_SOURCES
        console/WindowsConsole.h
        console/WindowsConsole.cpp
        ${LAUNCHER_SOURCES}
    )
endif()

qt_wrap_ui(LAUNCHER_UI
    ui/MainWindow.ui
    ui/setupwizard/PasteWizardPage.ui
    ui/setupwizard/AutoJavaWizardPage.ui
    ui/setupwizard/LoginWizardPage.ui
    ui/pages/global/AccountListPage.ui
    ui/pages/global/JavaPage.ui
    ui/pages/global/LauncherPage.ui
    ui/pages/global/APIPage.ui
    ui/pages/global/ProxyPage.ui
    ui/pages/global/ExternalToolsPage.ui
    ui/pages/instance/ExternalResourcesPage.ui
    ui/pages/instance/NotesPage.ui
    ui/pages/instance/LogPage.ui
    ui/pages/instance/ServersPage.ui
    ui/pages/instance/OtherLogsPage.ui
    ui/pages/instance/VersionPage.ui
    ui/pages/instance/ManagedPackPage.ui
    ui/pages/instance/WorldListPage.ui
    ui/pages/instance/ScreenshotsPage.ui
    ui/pages/modplatform/atlauncher/AtlOptionalModDialog.ui
    ui/pages/modplatform/atlauncher/AtlPage.ui
    ui/pages/modplatform/CustomPage.ui
    ui/pages/modplatform/ResourcePage.ui
    ui/pages/modplatform/flame/FlamePage.ui
    ui/pages/modplatform/legacy_ftb/Page.ui
    ui/pages/modplatform/import_ftb/ImportFTBPage.ui
    ui/pages/modplatform/ImportPage.ui
    ui/pages/modplatform/OptionalModDialog.ui
    ui/pages/modplatform/ftb/FtbPage.ui
    ui/pages/modplatform/modrinth/ModrinthPage.ui
    ui/pages/modplatform/technic/TechnicPage.ui
    ui/widgets/CustomCommands.ui
    ui/widgets/EnvironmentVariables.ui
    ui/widgets/InfoFrame.ui
    ui/widgets/ModFilterWidget.ui
    ui/widgets/SubTaskProgressBar.ui
    ui/widgets/AppearanceWidget.ui
    ui/widgets/MinecraftSettingsWidget.ui
    ui/widgets/JavaSettingsWidget.ui
    ui/dialogs/CopyInstanceDialog.ui
    ui/dialogs/CreateShortcutDialog.ui
    ui/dialogs/ProfileSetupDialog.ui
    ui/dialogs/ProgressDialog.ui
    ui/dialogs/NewInstanceDialog.ui
    ui/dialogs/NewComponentDialog.ui
    ui/dialogs/NewsDialog.ui
    ui/dialogs/ProfileSelectDialog.ui
    ui/dialogs/ExportInstanceDialog.ui
    ui/dialogs/ExportPackDialog.ui
    ui/dialogs/ExportToModListDialog.ui
    ui/dialogs/IconPickerDialog.ui
    ui/dialogs/ImportResourceDialog.ui
    ui/dialogs/MSALoginDialog.ui
    ui/dialogs/AboutDialog.ui
    ui/dialogs/ReviewMessageBox.ui
    ui/dialogs/ScrollMessageBox.ui
    ui/dialogs/BlockedModsDialog.ui
    ui/dialogs/ChooseProviderDialog.ui
    ui/dialogs/skins/SkinManageDialog.ui
    ui/dialogs/ChooseOfflineNameDialog.ui
)

qt_wrap_ui(PRISM_UPDATE_UI
    ui/dialogs/UpdateAvailableDialog.ui
)

if (NOT Apple)
    set (LAUNCHER_UI ${LAUNCHER_UI} ${PRISM_UPDATE_UI})
endif()

qt_add_resources(LAUNCHER_RESOURCES
    resources/backgrounds/backgrounds.qrc
    resources/multimc/multimc.qrc
    resources/pe_dark/pe_dark.qrc
    resources/pe_light/pe_light.qrc
    resources/pe_colored/pe_colored.qrc
    resources/pe_blue/pe_blue.qrc
    resources/breeze_dark/breeze_dark.qrc
    resources/breeze_light/breeze_light.qrc
    resources/OSX/OSX.qrc
    resources/iOS/iOS.qrc
    resources/flat/flat.qrc
    resources/documents/documents.qrc
    resources/shaders/shaders.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/../${Launcher_Branding_LogoQRC}"
)

qt_wrap_ui(PRISMUPDATER_UI
    updater/prismupdater/SelectReleaseDialog.ui
    ui/widgets/SubTaskProgressBar.ui
    ui/dialogs/ProgressDialog.ui
)

######## Windows resource files ########
if(WIN32)
    set(LAUNCHER_RCS ${CMAKE_CURRENT_BINARY_DIR}/../${Launcher_Branding_WindowsRC})
endif()

include(CompilerWarnings)

######## Precompiled Headers ###########

if(${Launcher_USE_PCH})
    message(STATUS "Using precompiled headers for applicable launcher targets")
    set(PRECOMPILED_HEADERS
        include/base.pch.hpp
        include/qtcore.pch.hpp
        include/qtgui.pch.hpp
    )
endif()

####### Targets ########

# Add executable
add_library(Launcher_logic STATIC ${LOGIC_SOURCES} ${LAUNCHER_SOURCES} ${LAUNCHER_UI} ${LAUNCHER_RESOURCES})
set_project_warnings(Launcher_logic
    "${Launcher_MSVC_WARNINGS}"
    "${Launcher_CLANG_WARNINGS}"
    "${Launcher_GCC_WARNINGS}")
target_include_directories(Launcher_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(Launcher_logic PUBLIC LAUNCHER_APPLICATION)

if(${Launcher_USE_PCH})
    target_precompile_headers(Launcher_logic PRIVATE ${PRECOMPILED_HEADERS})
endif()

target_link_libraries(Launcher_logic
    Launcher_murmur2
    nbt++
    ${ZLIB_LIBRARIES}
    qdcss
    BuildConfig
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(TARGET PkgConfig::libqrencode)
    target_link_libraries(Launcher_logic PkgConfig::libqrencode)
else()
    target_include_directories(Launcher_logic PRIVATE ${LIBQRENCODE_INCLUDE_DIR})
    target_link_libraries(Launcher_logic ${LIBQRENCODE_LIBRARIES})
endif()

if(TARGET PkgConfig::tomlplusplus)
    target_link_libraries(Launcher_logic PkgConfig::tomlplusplus)
else()
    target_link_libraries(Launcher_logic tomlplusplus::tomlplusplus)
endif()
if(TARGET PkgConfig::libarchive)
    target_link_libraries(Launcher_logic PkgConfig::libarchive)
else()
    target_link_libraries(Launcher_logic LibArchive::LibArchive)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
    Qt${QT_VERSION_MAJOR}::NetworkAuth
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${Launcher_QT_DBUS}
    ${Launcher_QT_LIBS}
)
target_link_libraries(Launcher_logic
    cmark::cmark
    LocalPeer
    Launcher_rainbow
)
if (TARGET ${Launcher_QT_DBUS})
    add_compile_definitions(WITH_QTDBUS)
endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@loader_path/../Frameworks/")

    if(Launcher_ENABLE_UPDATER)
        file(DOWNLOAD ${MACOSX_SPARKLE_DOWNLOAD_URL} ${CMAKE_BINARY_DIR}/Sparkle.tar.xz EXPECTED_HASH SHA256=${MACOSX_SPARKLE_SHA256})
        file(ARCHIVE_EXTRACT INPUT ${CMAKE_BINARY_DIR}/Sparkle.tar.xz DESTINATION ${CMAKE_BINARY_DIR}/frameworks/Sparkle)

        find_library(SPARKLE_FRAMEWORK Sparkle "${CMAKE_BINARY_DIR}/frameworks/Sparkle")
        add_compile_definitions(SPARKLE_ENABLED)
    endif()

    target_link_libraries(Launcher_logic
        "-framework AppKit"
        "-framework Carbon"
        "-framework Foundation"
        "-framework ApplicationServices"
    )
    if(Launcher_ENABLE_UPDATER)
        target_link_libraries(Launcher_logic ${SPARKLE_FRAMEWORK})
    endif()
endif()

target_link_libraries(Launcher_logic)

add_executable(${Launcher_Name} MACOSX_BUNDLE WIN32 main.cpp ${LAUNCHER_RCS})

if(${Launcher_USE_PCH})
    target_precompile_headers(${Launcher_Name} REUSE_FROM Launcher_logic)
endif()

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
    RUNTIME_DEPENDENCY_SET LAUNCHER_DEPENDENCY_SET
    BUNDLE DESTINATION "." COMPONENT Runtime
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
    RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
    FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR} COMPONENT Runtime
)

# Deploy PDBs
if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
    install(FILES $<TARGET_PDB_FILE:${Launcher_Name}> DESTINATION ${BINARY_DEST_DIR} OPTIONAL)
endif()

if(Launcher_BUILD_UPDATER)
    # Updater
    add_library(prism_updater_logic STATIC ${PRISMUPDATER_SOURCES} ${TASKS_SOURCES} ${PRISMUPDATER_UI})
    target_include_directories(prism_updater_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    if(${Launcher_USE_PCH})
        target_precompile_headers(prism_updater_logic PRIVATE ${PRECOMPILED_HEADERS})
    endif()

    target_link_libraries(prism_updater_logic
        ${ZLIB_LIBRARIES}
        BuildConfig
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        ${Launcher_QT_LIBS}
        cmark::cmark
    )
    if(TARGET PkgConfig::libarchive)
        target_link_libraries(prism_updater_logic PkgConfig::libarchive)
    else()
        target_link_libraries(prism_updater_logic LibArchive::LibArchive)
    endif()

    add_executable("${Launcher_Name}_updater" WIN32 updater/prismupdater/updater_main.cpp)
    target_sources("${Launcher_Name}_updater" PRIVATE updater/prismupdater/updater.exe.manifest)
    target_link_libraries("${Launcher_Name}_updater" prism_updater_logic)

    if(${Launcher_USE_PCH})
        target_precompile_headers("${Launcher_Name}_updater" REUSE_FROM prism_updater_logic)
    endif()

    if(DEFINED Launcher_APP_BINARY_NAME)
        set_target_properties("${Launcher_Name}_updater" PROPERTIES OUTPUT_NAME "${Launcher_APP_BINARY_NAME}_updater")
    endif()
    if(DEFINED Launcher_BINARY_RPATH)
        SET_TARGET_PROPERTIES("${Launcher_Name}_updater" PROPERTIES INSTALL_RPATH "${Launcher_BINARY_RPATH}")
    endif()

    install(TARGETS "${Launcher_Name}_updater"
        BUNDLE DESTINATION "." COMPONENT Runtime
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
        RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
        FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR} COMPONENT Runtime
    )

    # Deploy PDBs
    if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
        install(FILES $<TARGET_PDB_FILE:${Launcher_Name}_updater> DESTINATION ${BINARY_DEST_DIR} OPTIONAL)
    endif()
endif()

if(WIN32 OR (DEFINED Launcher_BUILD_FILELINKER AND Launcher_BUILD_FILELINKER))
    # File link
    add_library(filelink_logic STATIC ${LINKEXE_SOURCES})
    set_project_warnings(filelink_logic
    "${Launcher_MSVC_WARNINGS}"
    "${Launcher_CLANG_WARNINGS}"
    "${Launcher_GCC_WARNINGS}")

    target_include_directories(filelink_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    if(${Launcher_USE_PCH})
        target_precompile_headers(filelink_logic PRIVATE ${PRECOMPILED_HEADERS})
    endif()

    target_link_libraries(filelink_logic
        BuildConfig
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        # Qt${QT_VERSION_MAJOR}::Concurrent
        ${Launcher_QT_LIBS}
    )

    add_executable("${Launcher_Name}_filelink" WIN32 filelink/filelink_main.cpp)
    target_sources("${Launcher_Name}_filelink" PRIVATE filelink/filelink.exe.manifest)

    if(${Launcher_USE_PCH})
        target_precompile_headers("${Launcher_Name}_filelink" REUSE_FROM filelink_logic)
    endif()

    # HACK: Fix manifest issues with Ninja in release mode (and only release mode) and MSVC
    # I have no idea why this works or why it's needed. UPDATE THIS IF YOU EDIT THE MANIFEST!!! -@getchoo
    # Thank you 2018 CMake mailing list thread https://cmake.cmake.narkive.com/LnotZXus/conflicting-msvc-manifests
    if(MSVC)
        set_property(TARGET "${Launcher_Name}_filelink" PROPERTY LINK_FLAGS "/MANIFESTUAC:level='requireAdministrator'")
    endif()

    target_link_libraries("${Launcher_Name}_filelink" filelink_logic)

    if(DEFINED Launcher_APP_BINARY_NAME)
        set_target_properties("${Launcher_Name}_filelink" PROPERTIES OUTPUT_NAME "${Launcher_APP_BINARY_NAME}_filelink")
    endif()
    if(DEFINED Launcher_BINARY_RPATH)
        SET_TARGET_PROPERTIES("${Launcher_Name}_filelink" PROPERTIES INSTALL_RPATH "${Launcher_BINARY_RPATH}")
    endif()

    install(TARGETS "${Launcher_Name}_filelink"
        BUNDLE DESTINATION "." COMPONENT Runtime
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR} COMPONENT Runtime
        RUNTIME DESTINATION ${BINARY_DEST_DIR} COMPONENT Runtime
        FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR} COMPONENT Runtime
    )

    # Deploy PDBs
    if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
        install(FILES $<TARGET_PDB_FILE:${Launcher_Name}_filelink> DESTINATION ${BINARY_DEST_DIR} OPTIONAL)
    endif()
endif()

if (UNIX AND APPLE AND Launcher_ENABLE_UPDATER)
    # Add Sparkle updater
    # It has to be copied here instead of just allowing fixup_bundle to install it, otherwise essential parts of
    # the framework aren't installed
    install(DIRECTORY ${MACOSX_SPARKLE_DIR}/Sparkle.framework DESTINATION ${FRAMEWORK_DEST_DIR} USE_SOURCE_PERMISSIONS)
endif()

#### The bundle mess! ####
# Bundle utilities are used to complete packages for different platforms - they add all the libraries that would otherwise be missing on the target system.
# NOTE: it seems that this absolutely has to be here, and nowhere else.
if(WIN32 OR (UNIX AND APPLE))
    if(WIN32)
        set(QT_DEPLOY_TOOL_OPTIONS "--no-opengl-sw --no-quick-import --no-system-d3d-compiler --no-system-dxc-compiler --skip-plugin-types generic,networkinformation")
    endif()

    qt_generate_deploy_script(
        TARGET ${Launcher_Name}
        OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
        CONTENT "
        qt_deploy_runtime_dependencies(
            EXECUTABLE ${BINARY_DEST_DIR}/$<TARGET_FILE_NAME:${Launcher_Name}>
            BIN_DIR ${BINARY_DEST_DIR}
            LIBEXEC_DIR ${LIBRARY_DEST_DIR}
            LIB_DIR ${LIBRARY_DEST_DIR}
            PLUGINS_DIR ${PLUGIN_DEST_DIR}
            NO_OVERWRITE
            NO_TRANSLATIONS
            NO_COMPILER_RUNTIME
            DEPLOY_TOOL_OPTIONS ${QT_DEPLOY_TOOL_OPTIONS}
        )"
    )

    # Bundle our linked dependencies
    install(
        RUNTIME_DEPENDENCY_SET LAUNCHER_DEPENDENCY_SET
        COMPONENT bundle
        DIRECTORIES
            ${CMAKE_SYSTEM_LIBRARY_PATH}
            ${QT_LIBS_DIR}
            ${QT_LIBEXECS_DIR}
        PRE_EXCLUDE_REGEXES
            "^(api-ms-win|ext-ms)-.*\\.dll$"
            # FIXME: Why aren't these caught by the below regex???
            "^azure.*\\.dll$"
            "^vcruntime.*\\.dll$"
        POST_EXCLUDE_REGEXES
            "system32"
        LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
        RUNTIME DESTINATION ${BINARY_DEST_DIR}
        FRAMEWORK DESTINATION ${FRAMEWORK_DEST_DIR}
    )
    # Deploy Qt plugins
    install(
        SCRIPT ${QT_DEPLOY_SCRIPT}
        COMPONENT bundle
    )
    # FIXME: remove this crap once we stop using msys2
    if(MINGW)
        # i've not found a solution better than injecting the config vars like this...
        # with install(CODE" for everything everything just breaks
        install(CODE "
            set(QT_PLUGINS_DIR \"${QT_PLUGINS_DIR}\")
            set(QT_LIBS_DIR \"${QT_LIBS_DIR}\")
            set(QT_LIBEXECS_DIR \"${QT_LIBEXECS_DIR}\")
            set(CMAKE_SYSTEM_LIBRARY_PATH \"${CMAKE_SYSTEM_LIBRARY_PATH}\")
            set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
            "
            COMPONENT bundle)

        install(CODE [[
            file(GLOB QT_IMAGEFORMAT_DLLS "${QT_PLUGINS_DIR}/imageformats/*.dll")
            set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)
            file(GET_RUNTIME_DEPENDENCIES
                RESOLVED_DEPENDENCIES_VAR imageformatdeps
                LIBRARIES ${QT_IMAGEFORMAT_DLLS}
                DIRECTORIES
                    ${CMAKE_SYSTEM_LIBRARY_PATH}
                    ${QT_PLUGINS_DIR}
                    ${QT_LIBS_DIR}
                    ${QT_LIBEXECS_DIR}
                PRE_EXCLUDE_REGEXES
                    "^(api-ms-win|ext-ms)-.*\\.dll$"
                    # FIXME: Why aren't these caught by the below regex???
                    "^azure.*\\.dll$"
                    "^vcruntime.*\\.dll$"
                POST_EXCLUDE_REGEXES
                    "system32"
            )
            foreach(_lib ${imageformatdeps})
                file(INSTALL
                    DESTINATION ${CMAKE_INSTALL_PREFIX}
                    TYPE SHARED_LIBRARY
                    FOLLOW_SYMLINK_CHAIN
                    FILES ${_lib}
                )
            endforeach()
        ]]
        COMPONENT bundle)
    endif()

    # Add qt.conf - this makes Qt stop looking for things outside the bundle
    install(
        CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${RESOURCES_DEST_DIR}/qt.conf\" \" \")"
        COMPONENT bundle
    )
    # Add qtlogging.ini as a config file
    install(
        FILES "qtlogging.ini"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${RESOURCES_DEST_DIR}
        COMPONENT bundle
    )
endif()

find_program(CLANG_FORMAT clang-format OPTIONAL)
if(CLANG_FORMAT)
    message(STATUS "Creating clang-format target")
    add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT} -i --style=file:${CMAKE_SOURCE_DIR}/.clang-format ${LOGIC_SOURCES} ${LAUNCHER_SOURCES} ${PRISMUPDATER_SOURCES} ${LINKEXE_SOURCES} ${PRECOMPILED_HEADERS}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
else()
    message(WARNING "Unable to find `clang-format`. Not creating custom target")
endif()
