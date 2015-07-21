project(MultiMC-Logic)

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
	Exception.h

	# RW lock protected map
	RWStorage.h

	# A variable that has an implicit default value and keeps track of changes
	DefaultVariable.h

	# a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
	QObjectPtr.h

	# Resources
	resources/IconResourceHandler.cpp
	resources/IconResourceHandler.h
	resources/Resource.cpp
	resources/Resource.h
	resources/ResourceHandler.cpp
	resources/ResourceHandler.h
	resources/ResourceObserver.cpp
	resources/ResourceObserver.h
	resources/WebResourceHandler.cpp
	resources/WebResourceHandler.h
	resources/ResourceProxyModel.h
	resources/ResourceProxyModel.cpp

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
	auth/AuthSession.h
	auth/AuthSession.cpp
	auth/MojangAccountList.h
	auth/MojangAccountList.cpp
	auth/MojangAccount.h
	auth/MojangAccount.cpp
	auth/YggdrasilTask.h
	auth/YggdrasilTask.cpp
	auth/flows/AuthenticateTask.h
	auth/flows/AuthenticateTask.cpp
	auth/flows/RefreshTask.cpp
	auth/flows/RefreshTask.cpp
	auth/flows/ValidateTask.h
	auth/flows/ValidateTask.cpp

	# Game launch logic
	launch/steps/CheckJava.cpp
	launch/steps/CheckJava.h
	launch/steps/LaunchCommand.cpp
	launch/steps/LaunchCommand.h
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
	minecraft/OneSixUpdate.h
	minecraft/OneSixUpdate.cpp
	minecraft/OneSixInstance.h
	minecraft/OneSixInstance.cpp
	minecraft/LegacyUpdate.h
	minecraft/LegacyUpdate.cpp
	minecraft/LegacyInstance.h
	minecraft/LegacyInstance.cpp
	minecraft/LwjglVersionList.h
	minecraft/LwjglVersionList.cpp
	minecraft/SkinUtils.h
	minecraft/SkinUtils.cpp
	minecraft/GradleSpecifier.h
	minecraft/MinecraftProfile.cpp
	minecraft/MinecraftProfile.h
	minecraft/JarMod.cpp
	minecraft/JarMod.h
	minecraft/MinecraftInstance.cpp
	minecraft/MinecraftInstance.h
	minecraft/MinecraftVersion.cpp
	minecraft/MinecraftVersion.h
	minecraft/MinecraftVersionList.cpp
	minecraft/MinecraftVersionList.h
	minecraft/NullProfileStrategy.h
	minecraft/OneSixLibrary.cpp
	minecraft/OneSixLibrary.h
	minecraft/OneSixProfileStrategy.cpp
	minecraft/OneSixProfileStrategy.h
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

	# FTB
	ftb/OneSixFTBInstance.h
	ftb/OneSixFTBInstance.cpp
	ftb/LegacyFTBInstance.h
	ftb/LegacyFTBInstance.cpp
	ftb/FTBProfileStrategy.h
	ftb/FTBProfileStrategy.cpp
	ftb/FTBPlugin.h
	ftb/FTBPlugin.cpp

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
	java/JavaUtils.h
	java/JavaUtils.cpp
	java/JavaVersionList.h
	java/JavaVersionList.cpp
	java/JavaCheckerJob.h
	java/JavaCheckerJob.cpp

	# Assets
	minecraft/AssetsUtils.h
	minecraft/AssetsUtils.cpp

	# Forge and all things forge related
	forge/ForgeVersion.h
	forge/ForgeVersion.cpp
	forge/ForgeVersionList.h
	forge/ForgeVersionList.cpp
	forge/ForgeMirror.h
	forge/ForgeMirrors.h
	forge/ForgeMirrors.cpp
	forge/ForgeXzDownload.h
	forge/ForgeXzDownload.cpp
	forge/LegacyForge.h
	forge/LegacyForge.cpp
	forge/ForgeInstaller.h
	forge/ForgeInstaller.cpp

	# Liteloader and related things
	liteloader/LiteLoaderInstaller.h
	liteloader/LiteLoaderInstaller.cpp
	liteloader/LiteLoaderVersionList.h
	liteloader/LiteLoaderVersionList.cpp

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

# Add common library
add_library(MultiMC_logic STATIC ${LOGIC_SOURCES})

# Link
target_link_libraries(MultiMC_logic xz-embedded unpack200 iconfix libUtil LogicalGui ${QUAZIP_LIBRARIES}
	Qt5::Core Qt5::Xml Qt5::Widgets Qt5::Network Qt5::Concurrent
		${MultiMC_LINK_ADDITIONAL_LIBS})

add_dependencies(MultiMC_logic QuaZIP)
