#pragma once

#include "logic/minecraft/LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual QString getStatusbarDescription();
	virtual QString id() const;
	virtual void copy(const QDir &newDir);
};
