#include "LegacyFTBInstance.h"

LegacyFTBInstance::LegacyFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	LegacyInstance(rootDir, settings, parent)
{
}

QString LegacyFTBInstance::getStatusbarDescription()
{
	if (flags() & VersionBrokenFlag)
	{
		return "Legacy FTB: " + intendedVersionId() + " (broken)";
	}
	return "Legacy FTB: " + intendedVersionId();
}

QString LegacyFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}
