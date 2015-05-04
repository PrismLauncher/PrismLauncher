#include "MinecraftInstance.h"
#include <settings/Setting.h>
#include "settings/SettingsObject.h"
#include <pathutils.h>
#include "Env.h"
#include "minecraft/MinecraftVersionList.h"

// all of this because keeping things compatible with deprecated old settings
// if either of the settings {a, b} is true, this also resolves to true
class OrSetting : public Setting
{
	Q_OBJECT
public:
	OrSetting(QString id, std::shared_ptr<Setting> a, std::shared_ptr<Setting> b)
	:Setting({id}, false), m_a(a), m_b(b)
	{
	}
	virtual QVariant get() const
	{
		bool a = m_a->get().toBool();
		bool b = m_b->get().toBool();
		return a || b;
	}
	virtual void reset() {}
	virtual void set(QVariant value) {}
private:
	std::shared_ptr<Setting> m_a;
	std::shared_ptr<Setting> m_b;
};

MinecraftInstance::MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: BaseInstance(globalSettings, settings, rootDir)
{
	// Java Settings
	auto javaOverride = m_settings->registerSetting("OverrideJava", false);
	auto locationOverride = m_settings->registerSetting("OverrideJavaLocation", false);
	auto javaOrLocation = std::make_shared<OrSetting>("JavaOrLocationOverride", javaOverride, locationOverride);
	m_settings->registerSetting("OverrideJavaArgs", false);
	m_settings->registerOverride(globalSettings->getSetting("JavaPath"));
	m_settings->registerOverride(globalSettings->getSetting("JvmArgs"));

	// special!
	m_settings->registerPassthrough(globalSettings->getSetting("JavaTimestamp"), javaOrLocation);
	m_settings->registerPassthrough(globalSettings->getSetting("JavaVersion"), javaOrLocation);

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

#include "MinecraftInstance.moc"
