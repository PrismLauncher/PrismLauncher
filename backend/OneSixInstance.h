#pragma once

#include "BaseInstance.h"
#include <QStringList>
class FullVersion;

class LIBMULTIMC_EXPORT OneSixInstance : public BaseInstance
{
	Q_OBJECT
public:
	explicit OneSixInstance(const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
	virtual OneSixUpdate* doUpdate();
	virtual MinecraftProcess* prepareForLaunch ( QString user, QString session );
	virtual void cleanupAfterRun();
	
	virtual QString intendedVersionId() const;
	virtual bool setIntendedVersionId ( QString version );
	
	virtual QString currentVersionId() const;
	// virtual void setCurrentVersionId ( QString val ) {};
	
	virtual bool shouldUpdate() const;
	virtual void setShouldUpdate(bool val);
	
	/// reload the full version json file. return true on success!
	bool reloadFullVersion();
	/// get the current full version info
	QSharedPointer<FullVersion> getFullVersion();
private:
	QStringList processMinecraftArgs( QString user, QString session );
};