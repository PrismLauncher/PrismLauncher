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

void LegacyFTBInstance::copy(SettingsObjectPtr newSettings, const QDir& newDir)
{
	// set the target instance to be plain Legacy
	newSettings->set("InstanceType", "Legacy");
}

QString LegacyFTBInstance::typeName() const
{
	return tr("Legacy FTB");
}
