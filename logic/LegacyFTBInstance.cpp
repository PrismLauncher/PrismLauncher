#include "LegacyFTBInstance.h"

LegacyFTBInstance::LegacyFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	LegacyInstance(rootDir, settings, parent)
{
}

QString LegacyFTBInstance::getStatusbarDescription()
{
	return "Legacy FTB: " + intendedVersionId();
}

bool LegacyFTBInstance::menuActionEnabled(QString action_name) const
{
	return false;
}

QString LegacyFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}
