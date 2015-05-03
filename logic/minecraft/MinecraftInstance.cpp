#include "MinecraftInstance.h"
#include "settings/SettingsObject.h"
#include <pathutils.h>
#include "Env.h"
#include "minecraft/MinecraftVersionList.h"

MinecraftInstance::MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: BaseInstance(globalSettings, settings, rootDir)
{
	// Java Settings
	m_settings->registerSetting("OverrideJava", false);
	auto locationOverride = m_settings->registerSetting("OverrideJavaLocation", false);
	m_settings->registerSetting("OverrideJavaArgs", false);
	m_settings->registerOverride(globalSettings->getSetting("JavaPath"));
	m_settings->registerOverride(globalSettings->getSetting("JvmArgs"));

	// special!
	m_settings->registerPassthrough(globalSettings->getSetting("JavaTimestamp"), locationOverride);
	m_settings->registerPassthrough(globalSettings->getSetting("JavaVersion"), locationOverride);

	// Window Size
	m_settings->registerSetting("OverrideWindow", false);
	m_settings->registerOverride(globalSettings->getSetting("LaunchMaximized"));
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinWidth"));
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinHeight"));

	// Memory
	m_settings->registerSetting("OverrideMemory", false);
	m_settings->registerOverride(globalSettings->getSetting("MinMemAlloc"));
	m_settings->registerOverride(globalSettings->getSetting("MaxMemAlloc"));
	m_settings->registerOverride(globalSettings->getSetting("PermGen"));
}

QString MinecraftInstance::minecraftRoot() const
{
	QFileInfo mcDir(PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(instanceRoot(), ".minecraft"));

	if (dotMCDir.exists() && !mcDir.exists())
		return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

std::shared_ptr< BaseVersionList > MinecraftInstance::versionList() const
{
	return ENV.getVersionList("net.minecraft");
}
