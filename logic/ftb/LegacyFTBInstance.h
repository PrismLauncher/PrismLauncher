#pragma once

#include "minecraft/LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual QString id() const;
	virtual void copy(const QDir &newDir);
	virtual QString typeName() const;
};
