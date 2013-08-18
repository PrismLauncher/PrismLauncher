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
	QString mcJar() const;
	
	//! Path to the instance's mcbackup.jar
	QString mcBackup() const;
	
	//! Path to the instance's modlist file.
	QString modListFile() const;
	
	//////  Mod Lists  //////
	QSharedPointer<ModList> jarModList();
	QSharedPointer<ModList> coreModList();
	QSharedPointer<ModList> loaderModList();
	
	////// Directories //////
	QString savesDir() const;
	QString jarModsDir() const;
	QString binDir() const;
	QString mlModsDir() const;
	QString coreModsDir() const;
	QString resourceDir() const;
	
	/*!
	 * \brief Checks whether or not the currentVersion of the instance needs to be updated. 
	 *        If this returns true, updateCurrentVersion is called. In the 
	 *        standard instance, this is determined by checking a timestamp 
	 *        stored in the instance config file against the last modified time of Minecraft.jar.
	 * \return True if updateCurrentVersion() should be called.
	 */
	bool shouldUpdateCurrentVersion() const;
	
	/*!
	 * \brief Updates the current version. 
	 *        This function should first set the current version timestamp 
	 *        (setCurrentVersionTimestamp()) to the current time. Next, if 
	 *        keepCurrent is false, this function should check what the 
	 *        instance's current version is and call setCurrentVersion() to 
	 *        update it. This function will automatically be called when the 
	 *        instance is loaded if shouldUpdateCurrentVersion returns true.
	 * \param keepCurrent If true, only the version timestamp will be updated.
	 */
	void updateCurrentVersion(bool keepCurrent = false); 
	
	/*!
	 * Gets the last time that the current version was checked.
	 * This is checked against the last modified time on the jar file to see if
	 * the current version needs to be checked again.
	 */
	qint64 lastCurrentVersionUpdate() const;
	void setLastCurrentVersionUpdate(qint64 val);
	
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
	virtual QSharedPointer< QDialog > createModEditDialog ( QWidget* parent );
	
protected slots:
	virtual void jarModsChanged();
};