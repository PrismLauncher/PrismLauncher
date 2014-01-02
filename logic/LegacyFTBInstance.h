#pragma once

#include "LegacyInstance.h"

class LegacyFTBInstance : public LegacyInstance
{
	Q_OBJECT
public:
	explicit LegacyFTBInstance(const QString &rootDir, SettingsObject *settings,
							   QObject *parent = 0);
	virtual QString getStatusbarDescription();
	virtual bool menuActionEnabled(QString action_name) const;
};
