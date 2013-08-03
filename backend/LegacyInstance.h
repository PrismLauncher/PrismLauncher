#pragma once

#include "BaseInstance.h"

class LIBMULTIMC_EXPORT LegacyInstance : public BaseInstance
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
	
	////// Directories //////
	QString minecraftDir() const;
	QString instModsDir() const;
	QString binDir() const;
	QString savesDir() const;
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
	
	
	/*!
	 * The instance's current version.
	 * This value represents the instance's current version. If this value is 
	 * different from the intendedVersion, the instance should be updated.
	 * \warning Don't change this value unless you know what you're doing.
	 */
	QString currentVersion() const;
	void setCurrentVersion(QString val);

	//! The version of LWJGL that this instance uses.
	QString lwjglVersion() const;
	void setLWJGLVersion(QString val);
	
	/*!
	 * The version that the user has set for this instance to use.
	 * If this is not the same as currentVersion, the instance's game updater
	 * will be run on launch.
	 */
	virtual QString intendedVersionId();
	virtual bool setIntendedVersionId ( QString version );
	
	/*!
	 * Whether or not Minecraft should be downloaded when the instance is launched.
	 * This returns true if shouldForceUpdate game is true or if the intended and 
	 * current versions don't match.
	 */
	bool shouldUpdate() const;
	void setShouldUpdate(bool val);
	
	/// return a valid GameUpdateTask if an update is needed, return NULL otherwise
	virtual GameUpdateTask* doUpdate();
	
	/// prepare the instance for launch and return a constructed MinecraftProcess instance
	virtual MinecraftProcess* prepareForLaunch( QString user, QString session );
};