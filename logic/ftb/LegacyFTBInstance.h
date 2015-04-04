#pragma once

#include "logic/LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(const QString &rootDir, SettingsObject *settings,
							   QObject *parent = 0);
	virtual QString getStatusbarDescription();
	virtual QString id() const;
	virtual void copy(const QDir &newDir);
};
