/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INSTANCE_H
#define INSTANCE_H

#include <QObject>
#include <QDateTime>

#include <settingsobject.h>

#include "inifile.h"
#include "instancetypeinterface.h"
#include "instversionlist.h"

#include "libmmc_config.h"

class InstanceList;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and 
 * provides a standard interface for all instances.
 * 
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class LIBMULTIMC_EXPORT Instance : public QObject
{
	Q_OBJECT
	
	// Properties
	/*!
	 * The instance's ID.
	 * This is a unique identifier string that is, by default, set to the 
	 * instance's folder name. It's not always the instance's folder name, 
	 * however, as any class deriving from Instance can override the id() 
	 * method and change how the ID is determined. The instance's ID should 
	 * always remain constant. Undefined behavior results if an already loaded 
	 * instance's ID changes.
	 */
	Q_PROPERTY(QString id READ id STORED false)
	
	//! Path to the instance's root directory.
	Q_PROPERTY(QString rootDir READ rootDir)
	
	//! The name of the instance that is displayed to the user.
	Q_PROPERTY(QString name READ name WRITE setName)
	
	//! The instance's icon key.
	Q_PROPERTY(QString iconKey READ iconKey WRITE setIconKey)
	
	//! The instance's notes.
	Q_PROPERTY(QString notes READ notes WRITE setNotes)
	
	//! The instance's group.
	Q_PROPERTY(QString group READ group WRITE setGroup)
	
	/*!
	 * Whether or not the instance's minecraft.jar needs to be rebuilt.
	 * If this is true, when the instance launches, its jar mods will be 
	 * re-added to a fresh minecraft.jar file.
	 */
	Q_PROPERTY(bool shouldRebuild READ shouldRebuild WRITE setShouldRebuild)
	
	
	/*!
	 * The instance's current version.
	 * This value represents the instance's current version. If this value is 
	 * different from the intendedVersion, the instance should be updated.
	 * \warning Don't change this value unless you know what you're doing.
	 */
	Q_PROPERTY(QString currentVersion READ currentVersion WRITE setCurrentVersion)
	
	/*!
	 * The version that the user has set for this instance to use.
	 * If this is not the same as currentVersion, the instance's game updater
	 * will be run on launch.
	 */
	Q_PROPERTY(QString intendedVersion READ intendedVersion WRITE setIntendedVersion)
	
	//! The version of LWJGL that this instance uses.
	Q_PROPERTY(QString lwjglVersion READ lwjglVersion WRITE setLWJGLVersion)
	
	
	/*!
	 * Gets the time that the instance was last launched.
	 * Stored in milliseconds since epoch.
	 * This value is usually used for things like sorting instances by the time
	 * they were last launched.
	 */
	Q_PROPERTY(qint64 lastLaunch READ lastLaunch WRITE setLastLaunch)
	
	
	
	// Dirs
	//! Path to the instance's .minecraft folder.
	Q_PROPERTY(QString minecraftDir READ minecraftDir STORED false)
	
	//! Path to the instance's instMods folder.
	Q_PROPERTY(QString instModsDir READ instModsDir STORED false)
	
	//! Path to the instance's bin folder.
	Q_PROPERTY(QString binDir READ binDir STORED false)
	
	//! Path to the instance's saves folder.
	Q_PROPERTY(QString savesDir READ savesDir STORED false)
	
	//! Path to the instance's mods folder (.minecraft/mods)
	Q_PROPERTY(QString mlModsDir READ mlModsDir STORED false)
	
	//! Path to the instance's coremods folder.
	Q_PROPERTY(QString coreModsDir READ coreModsDir STORED false)
	
	//! Path to the instance's resources folder.
	Q_PROPERTY(QString resourceDir READ resourceDir STORED false)
	
	//! Path to the instance's screenshots folder.
	Q_PROPERTY(QString screenshotsDir READ screenshotsDir STORED false)
	
	//! Path to the instance's texturepacks folder.
	Q_PROPERTY(QString texturePacksDir READ texturePacksDir STORED false)
	
	
	// Files
	//! Path to the instance's minecraft.jar
	Q_PROPERTY(QString mcJar READ mcJar STORED false)
	
	//! Path to the instance's mcbackup.jar
	Q_PROPERTY(QString mcBackup READ mcBackup STORED false)
	
	//! Path to the instance's config file.
	Q_PROPERTY(QString configFile READ configFile STORED false)
	
	//! Path to the instance's modlist file.
	Q_PROPERTY(QString modListFile READ modListFile STORED false)
	
public:
	explicit Instance(const QString &rootDir, QObject *parent = 0);
	
	// Please, for the sake of my (and everyone else's) sanity, at least keep this shit
	// *somewhat* organized. Also, documentation is semi-important here. Please don't
	// leave undocumented stuff behind.
	// As a side-note, doxygen processes comments for accessor functions and 
	// properties separately, so please document properties in the massive block of
	// Q_PROPERTY declarations above rather than documenting their accessors.
	
	
	//////// STUFF ////////
	virtual QString id() const;
	
	virtual QString rootDir() const;
	
	/*!
	 * \brief Gets the instance list that this instance is a part of. 
	 *        Returns NULL if this instance is not in a list 
	 *        (the parent is not an InstanceList).
	 * \return A pointer to the InstanceList containing this instance. 
	 */
	virtual InstanceList *instList() const;
	
	
	//////// INSTANCE INFO ////////
	
	//// General Info ////
	virtual QString name() { return settings().get("name").toString(); }
	virtual void setName(QString val)
	{
		settings().set("name", val);
		emit propertiesChanged(this);
	}
	
	virtual QString iconKey() const { return settings().get("iconKey").toString(); }
	virtual void setIconKey(QString val)
	{
		settings().set("iconKey", val);
		emit propertiesChanged(this);
	}
	
	virtual QString notes() const { return settings().get("notes").toString(); }
	virtual void setNotes(QString val) { settings().set("notes", val); }
	
	virtual QString group() const { return m_group; }
	virtual void setGroup(QString val)
	{
		m_group = val;
		emit propertiesChanged(this);
	}
	
	virtual bool shouldRebuild() const { return settings().get("NeedsRebuild").toBool(); }
	virtual void setShouldRebuild(bool val) { settings().set("NeedsRebuild", val); }
	
	
	//// Version Stuff ////
	
	virtual QString currentVersion() { return settings().get("JarVersion").toString(); }
	virtual void setCurrentVersion(QString val) { settings().set("JarVersion", val); }
	
	virtual QString lwjglVersion() { return settings().get("LwjglVersion").toString(); }
	virtual void setLWJGLVersion(QString val) { settings().set("LwjglVersion", val); }
	
	virtual QString intendedVersion() { return settings().get("IntendedJarVersion").toString(); }
	virtual void setIntendedVersion(QString val) { settings().set("IntendedJarVersion", val); }
	
	
	
	//// Timestamps ////
	
	virtual qint64 lastLaunch() { return settings().get("lastLaunchTime").value<qint64>(); }
	virtual void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch()) 
	{
		settings().set("lastLaunchTime", val);
		emit propertiesChanged(this);
	}
	
	
	////// Directories //////
	QString minecraftDir() const;
	QString instModsDir() const;
	QString binDir() const;
	QString savesDir() const;
	QString mlModsDir() const;
	QString coreModsDir() const;
	QString resourceDir() const;
	QString screenshotsDir() const;
	QString texturePacksDir() const;
	
	
	////// Files //////
	QString mcJar() const;
	QString mcBackup() const;
	QString configFile() const;
	QString modListFile() const;
	
	
	//////// LISTS, LISTS, AND MORE LISTS ////////
	/*!
	 * \brief Gets a pointer to this instance's version list.
	 * \return A pointer to the available version list for this instance.
	 */
	virtual InstVersionList *versionList() const = 0;
	
	
	
	//////// INSTANCE TYPE STUFF ////////
	
	/*!
	 * \brief Returns a pointer to this instance's type.
	 * \return A pointer to this instance's type interface.
	 */
	virtual const InstanceTypeInterface *instanceType() const = 0;
	
	
	//////// OTHER FUNCTIONS ////////
	
	//// Version System ////
	
	/*!
	 * \brief Checks whether or not the currentVersion of the instance needs to be updated. 
	 *        If this returns true, updateCurrentVersion is called. In the 
	 *        standard instance, this is determined by checking a timestamp 
	 *        stored in the instance config file against the last modified time of Minecraft.jar.
	 * \return True if updateCurrentVersion() should be called.
	 */
	virtual bool shouldUpdateCurrentVersion() = 0;
	
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
	virtual void updateCurrentVersion(bool keepCurrent = false) = 0; 
	
	
	//// Settings System ////
	
	/*!
	 * \brief Gets this instance's settings object.
	 * This settings object stores instance-specific settings.
	 * \return A pointer to this instance's settings object.
	 */
	virtual SettingsObject &settings() const;
	
signals:
	/*!
	 * \brief Signal emitted when properties relevant to the instance view change
	 */
	void propertiesChanged(Instance * inst);
	
private:
	QString m_rootDir;
	QString m_group;
	SettingsObject *m_settings;
};

// pointer for lazy people
typedef QSharedPointer<Instance> InstancePtr;

#endif // INSTANCE_H
