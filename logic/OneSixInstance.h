#pragma once

#include "BaseInstance.h"
#include <QStringList>
class OneSixVersion;
class BaseUpdate;
class ModList;

class OneSixInstance : public BaseInstance
{
	Q_OBJECT
public:
	explicit OneSixInstance(const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
	
	
	//////  Mod Lists  //////
	std::shared_ptr<ModList> loaderModList();
	std::shared_ptr<ModList> resourcePackList();
	
	////// Directories //////
	QString resourcePacksDir() const;
	QString loaderModsDir() const;
	virtual QString instanceConfigFolder() const;
	
	virtual BaseUpdate* doUpdate();
	virtual MinecraftProcess* prepareForLaunch ( LoginResponse response );
	virtual void cleanupAfterRun();
	
	virtual QString intendedVersionId() const;
	virtual bool setIntendedVersionId ( QString version );
	
	virtual QString currentVersionId() const;
	// virtual void setCurrentVersionId ( QString val ) {};
	
	virtual bool shouldUpdate() const;
	virtual void setShouldUpdate(bool val);
	
	virtual QDialog * createModEditDialog ( QWidget* parent );
	
	/// reload the full version json file. return true on success!
	bool reloadFullVersion();
	/// get the current full version info
	std::shared_ptr<OneSixVersion> getFullVersion();
	/// revert the current custom version back to base
	bool revertCustomVersion();
	/// customize the current base version
	bool customizeVersion();
	/// is the current version original, or custom?
	bool versionIsCustom();
	
	virtual QString defaultBaseJar() const;
	virtual QString defaultCustomBaseJar() const;
	
	virtual bool menuActionEnabled ( QString action_name ) const;
	virtual QString getStatusbarDescription();
private:
	QStringList processMinecraftArgs( LoginResponse response );
};