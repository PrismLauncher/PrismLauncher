#pragma once

#include "OneSixInstance.h"

class NostalgiaInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit NostalgiaInstance(const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
	virtual QString getStatusbarDescription();
};
