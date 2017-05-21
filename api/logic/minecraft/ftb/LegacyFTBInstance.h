#pragma once

#include "minecraft/legacy/LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	QString id() const override;
	void copy(SettingsObjectPtr newSettings, const QDir &newDir) override;
	QString typeName() const override;
	bool canExport() const override
	{
		return false;
	}
};
