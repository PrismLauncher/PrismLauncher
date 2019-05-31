project(MultiMC_logic)

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

    # WARNING: globals live here
    Env.h
    Env.cpp

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
)

add_unit_test(FileSystem
    SOURCES FileSystem_test.cpp
    LIBS MultiMC_logic
    DATA testdata
    )

add_unit_test(GZip
    SOURCES GZip_test.cpp
    LIBS MultiMC_logic
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
    net/URLConstants.cpp
    net/URLConstants.h
    net/Validator.h
)

# Game launch logic
set(LAUNCH_SOURCES
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
    LIBS MultiMC_logic
    DATA updater/testdata
    )

add_unit_test(DownloadTask
    SOURCES updater/DownloadTask_test.cpp
    LIBS MultiMC_logic
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
    icons/IIconList.h
    icons/IIconList.cpp
    icons/IconUtils.h
    icons/IconUtils.cpp
)

# Minecraft services status checker
set(STATUS_SOURCES
    # Status system
    status/StatusChecker.h
    status/StatusChecker.cpp
)

# Support for Minecraft instances and launch
set(MINECRAFT_SOURCES
    # Minecraft support
    minecraft/auth/AuthSession.h
    minecraft/auth/AuthSession.cpp
    minecraft/auth/MojangAccountList.h
    minecraft/auth/MojangAccountList.cpp
    minecraft/auth/MojangAccount.h
    minecraft/auth/MojangAccount.cpp
    minecraft/auth/YggdrasilTask.h
    minecraft/auth/YggdrasilTask.cpp
    minecraft/auth/flows/AuthenticateTask.h
    minecraft/auth/flows/AuthenticateTask.cpp
    minecraft/auth/flows/RefreshTask.cpp
    minecraft/auth/flows/RefreshTask.cpp
    minecraft/auth/flows/ValidateTask.h
    minecraft/auth/flows/ValidateTask.cpp
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
    minecraft/launch/CreateServerResourcePacksFolder.cpp
    minecraft/launch/CreateServerResourcePacksFolder.h
    minecraft/launch/ModMinecraftJar.cpp
    minecraft/launch/ModMinecraftJar.h
    minecraft/launch/DirectJavaLaunch.cpp
    minecraft/launch/DirectJavaLaunch.h
    minecraft/launch/ExtractNatives.cpp
    minecraft/launch/ExtractNatives.h
    minecraft/launch/LauncherPartLaunch.cpp
    minecraft/launch/LauncherPartLaunch.h
    minecraft/launch/PrintInstanceInfo.cpp
    minecraft/launch/PrintInstanceInfo.h
    minecraft/launch/ReconstructAssets.cpp
    minecraft/launch/ReconstructAssets.h
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
    minecraft/ComponentList.cpp
    minecraft/ComponentList.h
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
    minecraft/Mod.h
    minecraft/Mod.cpp
    minecraft/SimpleModList.h
    minecraft/SimpleModList.cpp
    minecraft/World.h
    minecraft/World.cpp
    minecraft/WorldList.h
    minecraft/WorldList.cpp

    # Assets
    minecraft/AssetsUtils.h
    minecraft/AssetsUtils.cpp

    # Forge and all things forge related
    minecraft/forge/ForgeXzDownload.h
    minecraft/forge/ForgeXzDownload.cpp

    # Skin upload utilities
    minecraft/SkinUpload.cpp
    minecraft/SkinUpload.h
    )

add_unit_test(GradleSpecifier
    SOURCES minecraft/GradleSpecifier_test.cpp
    LIBS MultiMC_logic
    )

add_unit_test(MojangVersionFormat
    SOURCES minecraft/MojangVersionFormat_test.cpp
    LIBS MultiMC_logic
    DATA minecraft/testdata
    )

add_unit_test(Library
    SOURCES minecraft/Library_test.cpp
    LIBS MultiMC_logic
    )

# FIXME: shares data with FileSystem test
add_unit_test(SimpleModList
    SOURCES minecraft/SimpleModList_test.cpp
    DATA testdata
    LIBS MultiMC_logic
    )

add_unit_test(ParseUtils
    SOURCES minecraft/ParseUtils_test.cpp
    LIBS MultiMC_logic
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
    LIBS MultiMC_logic
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
    LIBS MultiMC_logic
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
    modplatform/ftb/FtbPackFetchTask.h
    modplatform/ftb/FtbPackFetchTask.cpp
    modplatform/ftb/FtbPackInstallTask.h
    modplatform/ftb/FtbPackInstallTask.cpp

    modplatform/ftb/FtbPrivatePackManager.h
    modplatform/ftb/FtbPrivatePackManager.cpp

    modplatform/ftb/PackHelpers.h
)

set(FLAME_SOURCES
    # Flame
    modplatform/flame/PackManifest.h
    modplatform/flame/PackManifest.cpp
    modplatform/flame/FileResolvingTask.h
    modplatform/flame/FileResolvingTask.cpp
)

add_unit_test(Index
    SOURCES meta/Index_test.cpp
    LIBS MultiMC_logic
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
    ${STATUS_SOURCES}
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
)

message(STATUS "FOO! ${LOGIC_SOURCES}")

add_library(MultiMC_logic SHARED ${LOGIC_SOURCES})
set_target_properties(MultiMC_logic PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(MultiMC_logic)

# Link
target_link_libraries(MultiMC_logic xz-embedded MultiMC_unpack200 systeminfo MultiMC_quazip MultiMC_classparser ${NBT_NAME} ${ZLIB_LIBRARIES})
target_link_libraries(MultiMC_logic Qt5::Core Qt5::Xml Qt5::Network Qt5::Concurrent)

# Mark and export headers
target_include_directories(MultiMC_logic PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE "${ZLIB_INCLUDE_DIRS}")

# Install it
install(
    TARGETS MultiMC_logic
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)
