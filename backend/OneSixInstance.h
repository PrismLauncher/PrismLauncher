#pragma once

#include "BaseInstance.h"
class LIBMULTIMC_EXPORT OneSixInstance : public BaseInstance
{
	Q_OBJECT
public:
	explicit OneSixInstance(const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
	virtual GameUpdateTask* doUpdate();
	virtual MinecraftProcess* prepareForLaunch ( QString user, QString session );
	
	virtual bool setIntendedVersionId ( QString version );
	virtual QString intendedVersionId();
	
};