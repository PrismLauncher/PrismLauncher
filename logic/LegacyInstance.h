#pragma once

#include "BaseInstance.h"

class ModList;
class BaseUpdate;

class LegacyInstance : public BaseInstance
{
	Q_OBJECT
public:
	
	explicit LegacyInstance(const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
	
	/// Path to the instance's minecraft.jar
	QString runnableJar() const;
	
	//! Path to the instance's modlist file.
	QString modListFile() const;
	
	//////  Mod Lists  //////
	QSharedPointer<ModList> jarModList();
	QSharedPointer<ModList> coreModList();
	QSharedPointer<ModList> loaderModList();
	QSharedPointer<ModList> texturePackList();
	
	////// Directories //////
	QString savesDir() const;
	QString texturePackDir() const;
	QString jarModsDir() const;
	QString binDir() const;
	QString mlModsDir() const;
	QString coreModsDir() const;
	QString resourceDir() const;
	virtual QString instanceConfigFolder() const;
	
	/*!
	 * Whether or not the instance's minecraft.jar needs to be rebuilt.
	 * If this is true, when the instance launches, its jar mods will be 
	 * re-added to a fresh minecraft.jar file.
	 */
	bool shouldRebuild() const;
	void setShouldRebuild(bool val);
	
	virtual QString currentVersionId() const;
	virtual void setCurrentVersionId(QString val);
	
	//! The version of LWJGL that this instance uses.
	QString lwjglVersion() const;
	/// st the version of LWJGL libs this instance will use
	void setLWJGLVersion(QString val);
	
	virtual QString intendedVersionId() const;
	virtual bool setIntendedVersionId ( QString version );
	
	virtual bool shouldUpdate() const;
	virtual void setShouldUpdate(bool val);
	virtual BaseUpdate* doUpdate();
	
	virtual MinecraftProcess* prepareForLaunch( QString user, QString session );
	virtual void cleanupAfterRun();
	virtual QDialog * createModEditDialog ( QWidget* parent );
	
	virtual QString defaultBaseJar() const;
	virtual QString defaultCustomBaseJar() const;
	
	bool menuActionEnabled ( QString action_name ) const;
	virtual QString getStatusbarDescription();
	
protected slots:
	virtual void jarModsChanged();
};