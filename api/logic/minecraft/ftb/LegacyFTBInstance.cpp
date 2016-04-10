#include "LegacyFTBInstance.h"
#include <settings/INISettingsObject.h>
#include <QDir>

LegacyFTBInstance::LegacyFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir) :
	LegacyInstance(globalSettings, settings, rootDir)
{
}

QString LegacyFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}

void LegacyFTBInstance::copy(const QDir &newDir)
{
	// set the target instance to be plain Legacy
	INISettingsObject settings_obj(newDir.absoluteFilePath("instance.cfg"));
	settings_obj.registerSetting("InstanceType", "Legacy");
	QString inst_type = settings_obj.get("InstanceType").toString();
	settings_obj.set("InstanceType", "Legacy");
}

QString LegacyFTBInstance::typeName() const
{
	return tr("Legacy FTB");
}
