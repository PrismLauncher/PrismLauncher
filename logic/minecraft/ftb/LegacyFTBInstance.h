#pragma once

#include "minecraft/legacy/LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual QString id() const;
	virtual void copy(const QDir &newDir);
	virtual QString typeName() const;
	bool canExport() const override
	{
		return false;
	}
};
