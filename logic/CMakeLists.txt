project(MultiMC_logic)

set(LOGIC_SOURCES
	# LOGIC - Base classes and infrastructure
	BaseInstaller.h
	BaseInstaller.cpp
	BaseVersionList.h
	BaseVersionList.cpp
	InstanceList.h
	InstanceList.cpp
	BaseVersion.h
	BaseInstance.h
	BaseInstance.cpp
	NullInstance.h
	MMCZip.h
	MMCZip.cpp
	MMCStrings.h
	MMCStrings.cpp
	BaseConfigObject.h
	BaseConfigObject.cpp
	AbstractCommonModel.h
	AbstractCommonModel.cpp
	TypeMagic.h

	# Prefix tree where node names are strings between separators
	SeparatorPrefixTree.h

	# WARNING: globals live here
	Env.h
	Env.cpp

	# JSON parsing helpers
	Json.h
	Json.cpp

	FileSystem.h
	FileSystem.cpp
	DesktopServices.h
	DesktopServices.cpp

	Exception.h

	# RW lock protected map
	RWStorage.h

	# A variable that has an implicit default value and keeps track of changes
	DefaultVariable.h

	# a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
	QObjectPtr.h

	# Resources
	resources/Resource.cpp
	resources/Resource.h
	resources/ResourceHandler.cpp
	resources/ResourceHandler.h
	resources/ResourceObserver.cpp
	resources/ResourceObserver.h
	resources/ResourceProxyModel.h
	resources/ResourceProxyModel.cpp

	# Path matchers
	pathmatcher/FSTreeMatcher.h
	pathmatcher/IPathMatcher.h
	pathmatcher/MultiMatcher.h
	pathmatcher/RegexpMatcher.h

	# Compression support
	GZip.h
	GZip.cpp

	# Command line parameter parsing
	Commandline.h
	Commandline.cpp

	# Version number string support
	Version.h
	Version.cpp

	# network stuffs
	net/NetAction.h
	net/MD5EtagDownload.h
	net/MD5EtagDownload.cpp
	net/ByteArrayDownload.h
	net/ByteArrayDownload.cpp
	net/CacheDownload.h
	net/CacheDownload.cpp
	net/NetJob.h
	net/NetJob.cpp
	net/HttpMetaCache.h
	net/HttpMetaCache.cpp
	net/PasteUpload.h
	net/PasteUpload.cpp
	net/URLConstants.h
	net/URLConstants.cpp

	# Yggdrasil login stuff
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

	# Game launch logic
	launch/steps/CheckJava.cpp
	launch/steps/CheckJava.h
	launch/steps/LaunchMinecraft.cpp
	launch/steps/LaunchMinecraft.h
	launch/steps/ModMinecraftJar.cpp
	launch/steps/ModMinecraftJar.h
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
	launch/LoggedProcess.cpp
	launch/LoggedProcess.h
	launch/MessageLevel.cpp
	launch/MessageLevel.h

	# Update system
	updater/GoUpdate.h
	updater/GoUpdate.cpp
	updater/UpdateChecker.h
	updater/UpdateChecker.cpp
	updater/DownloadTask.h
	updater/DownloadTask.cpp

	# Notifications - short warning messages
	notifications/NotificationChecker.h
	notifications/NotificationChecker.cpp

	# News System
	news/NewsChecker.h
	news/NewsChecker.cpp
	news/NewsEntry.h
	news/NewsEntry.cpp

	# Status system
	status/StatusChecker.h
	status/StatusChecker.cpp

	# Minecraft support
	minecraft/onesix/OneSixUpdate.h
	minecraft/onesix/OneSixUpdate.cpp
	minecraft/onesix/OneSixInstance.h
	minecraft/onesix/OneSixInstance.cpp
	minecraft/onesix/OneSixProfileStrategy.cpp
	minecraft/onesix/OneSixProfileStrategy.h
	minecraft/onesix/OneSixVersionFormat.cpp
	minecraft/onesix/OneSixVersionFormat.h
	minecraft/legacy/LegacyUpdate.h
	minecraft/legacy/LegacyUpdate.cpp
	minecraft/legacy/LegacyInstance.h
	minecraft/legacy/LegacyInstance.cpp
	minecraft/legacy/LwjglVersionList.h
	minecraft/legacy/LwjglVersionList.cpp
	minecraft/SkinUtils.h
	minecraft/SkinUtils.cpp
	minecraft/GradleSpecifier.h
	minecraft/MinecraftProfile.cpp
	minecraft/MinecraftProfile.h
	minecraft/MojangVersionFormat.cpp
	minecraft/MojangVersionFormat.h
	minecraft/JarMod.h
	minecraft/MinecraftInstance.cpp
	minecraft/MinecraftInstance.h
	minecraft/MinecraftVersion.cpp
	minecraft/MinecraftVersion.h
	minecraft/MinecraftVersionList.cpp
	minecraft/MinecraftVersionList.h
	minecraft/NullProfileStrategy.h
	minecraft/OneSixRule.cpp
	minecraft/OneSixRule.h
	minecraft/OpSys.cpp
	minecraft/OpSys.h
	minecraft/ParseUtils.cpp
	minecraft/ParseUtils.h
	minecraft/ProfileUtils.cpp
	minecraft/ProfileUtils.h
	minecraft/ProfileStrategy.h
	minecraft/RawLibrary.cpp
	minecraft/RawLibrary.h
	minecraft/VersionBuildError.h
	minecraft/VersionFile.cpp
	minecraft/VersionFile.h
	minecraft/ProfilePatch.h
	minecraft/VersionSource.h
	minecraft/VersionFilterData.h
	minecraft/VersionFilterData.cpp
	minecraft/Mod.h
	minecraft/Mod.cpp
	minecraft/ModList.h
	minecraft/ModList.cpp
	minecraft/World.h
	minecraft/World.cpp
	minecraft/WorldList.h
	minecraft/WorldList.cpp

	# FTB
	minecraft/ftb/OneSixFTBInstance.h
	minecraft/ftb/OneSixFTBInstance.cpp
	minecraft/ftb/LegacyFTBInstance.h
	minecraft/ftb/LegacyFTBInstance.cpp
	minecraft/ftb/FTBProfileStrategy.h
	minecraft/ftb/FTBProfileStrategy.cpp
	minecraft/ftb/FTBPlugin.h
	minecraft/ftb/FTBPlugin.cpp

	# A Recursive file system watcher
	RecursiveFileSystemWatcher.h
	RecursiveFileSystemWatcher.cpp

	# the screenshots feature
	screenshots/Screenshot.h
	screenshots/ImgurUpload.h
	screenshots/ImgurUpload.cpp
	screenshots/ImgurAlbumCreation.h
	screenshots/ImgurAlbumCreation.cpp

	# Icons
	icons/MMCIcon.h
	icons/MMCIcon.cpp
	icons/IconList.h
	icons/IconList.cpp

	# Tasks
	tasks/Task.h
	tasks/Task.cpp
	tasks/ThreadTask.h
	tasks/ThreadTask.cpp
	tasks/SequentialTask.h
	tasks/SequentialTask.cpp

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

	# Java related code
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

	# Assets
	minecraft/AssetsUtils.h
	minecraft/AssetsUtils.cpp

	# Forge and all things forge related
	minecraft/forge/ForgeVersion.h
	minecraft/forge/ForgeVersion.cpp
	minecraft/forge/ForgeVersionList.h
	minecraft/forge/ForgeVersionList.cpp
	minecraft/forge/ForgeMirror.h
	minecraft/forge/ForgeMirrors.h
	minecraft/forge/ForgeMirrors.cpp
	minecraft/forge/ForgeXzDownload.h
	minecraft/forge/ForgeXzDownload.cpp
	minecraft/forge/LegacyForge.h
	minecraft/forge/LegacyForge.cpp
	minecraft/forge/ForgeInstaller.h
	minecraft/forge/ForgeInstaller.cpp

	# Liteloader and related things
	minecraft/liteloader/LiteLoaderInstaller.h
	minecraft/liteloader/LiteLoaderInstaller.cpp
	minecraft/liteloader/LiteLoaderVersionList.h
	minecraft/liteloader/LiteLoaderVersionList.cpp

	# Translations
	trans/TranslationDownloader.h
	trans/TranslationDownloader.cpp

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
################################ COMPILE ################################

# we need zlib
find_package(ZLIB REQUIRED)

add_library(MultiMC_logic SHARED ${LOGIC_SOURCES})
set_target_properties(MultiMC_logic PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(MultiMC_logic)

# Link
target_link_libraries(MultiMC_logic xz-embedded unpack200 iconfix LogicalGui ${QUAZIP_LIBRARIES} nbt++ ${ZLIB_LIBRARIES})
qt5_use_modules(MultiMC_logic Core Xml Widgets Network Concurrent)
add_dependencies(MultiMC_logic QuaZIP)

# Mark and export headers
target_include_directories(MultiMC_logic PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE "${ZLIB_INCLUDE_DIRS}")
