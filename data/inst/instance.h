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

#include "data/appsettings.h"
#include "data/inifile.h"

#define DEFINE_OVERRIDDEN_SETTING_ADVANCED(funcName, cfgEntryName, typeName) \
	typeName get ## funcName() const { return getField(cfgEntryName, settings->get ## funcName()).value<typeName>(); }

#define DEFINE_OVERRIDDEN_SETTING(funcName, typeName) \
	DEFINE_OVERRIDDEN_SETTING_ADVANCED(funcName, STR_VAL(funcName), typeName)

class InstanceList;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and 
 * provides a standard interface for all instances.
 * 
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class Instance : public SettingsBase
{
	Q_OBJECT
public:
	explicit Instance(const QString &rootDir, QObject *parent = 0);
	
	// Please, for the sake of my (and everyone else's) sanity, at least keep this shit
	// *somewhat* organized. Also, documentation is semi-important here. Please don't
	// leave undocumented stuff behind.
	
	
	//////// STUFF ////////
	
	/*!
	 * \brief Get the instance's ID. 
	 *        This is a unique identifier string that is, by default, set to the
	 *        instance's folder name. It's not always the instance's folder name, 
	 *        however, as any class deriving from Instance can override the id() 
	 *        method and change how the ID is determined. The instance's ID 
	 *        should always remain constant. Undefined behavior results if an 
	 *        already loaded instance's ID changes.
	 * 
	 * \return The instance's ID.
	 */
	virtual QString id();
	
	/*!
	 * \brief Gets the path to the instance's root directory.
	 * \return  The path to the instance's root directory.
	 */
	virtual QString rootDir();
	
	/*!
	 * \brief Gets the instance list that this instance is a part of. 
	 *        Returns NULL if this instance is not in a list 
	 *        (the parent is not an InstanceList).
	 * \return A pointer to the InstanceList containing this instance. 
	 */
	virtual InstanceList *instList();
	
	
	//////// FIELDS AND SETTINGS ////////
	// Fields are options stored in the instance's config file that are specific
	// to instances (not a part of SettingsBase). Settings are things overridden
	// from SettingsBase.
	
	
	////// Fields //////
	
	//// General Info ////
	
	/*!
	 * \brief Gets this instance's name.
	 *        This is the name that will be displayed to the user.
	 * \return The instance's name.
	 * \sa setName
	 */
	virtual QString name() { return getField("name", "Unnamed Instance").value<QString>(); }
	
	/*!
	 * \brief Sets the instance's name
	 * \param val The instance's new name.
	 */
	virtual void setName(QString val) { setField("name", val); }
	
	/*!
	 * \brief Gets the instance's icon key.
	 * \return The instance's icon key.
	 * \sa setIconKey()
	 */
	virtual QString iconKey() const { return getField("iconKey", "default").value<QString>(); }
	
	/*!
	 * \brief Sets the instance's icon key.
	 * \param val The new icon key.
	 */
	virtual void setIconKey(QString val) { setField("iconKey", val); }
	
	
	/*!
	 * \brief Gets the instance's notes.
	 * \return The instances notes.
	 */
	virtual QString notes() const { return getField("notes", "").value<QString>(); }
	
	/*!
	 * \brief Sets the instance's notes.
	 * \param val The instance's new notes.
	 */
	virtual void setNotes(QString val) { setField("notes", val); }
	
	
	/*!
	 * \brief Checks if	the instance's minecraft.jar needs to be rebuilt.
	 *        If this is true, the instance's mods will be reinstalled to its
	 *        minecraft.jar file. This value is automatically set to true when
	 *        the jar mod list changes.
	 * \return Whether or not the instance's jar file should be rebuilt.
	 */
	virtual bool shouldRebuild() const { return getField("NeedsRebuild", false).value<bool>(); }
	
	/*!
	 * \brief Sets whether or not the instance's minecraft.jar needs to be rebuilt.
	 * \param val Whether the instance's minecraft needs to be rebuilt or not.
	 */
	virtual void setShouldRebuild(bool val) { setField("NeedsRebuild", val); }
	
	
	//// Version Stuff ////
	
	/*!
	 * \brief Sets the instance's current version.
	 *        This value represents the instance's current version. If this value
	 *        is different from intendedVersion(), the instance should be updated.
	 *        This value is updated by the updateCurrentVersion() function.
	 * \return A string representing the instance's current version.
	 */
	virtual QString currentVersion() { return getField("JarVersion", "Unknown").value<QString>(); }
	
	/*!
	 * \brief Sets the instance's current version.
	 *        This is used to keep track of the instance's current version. Don't
	 *        mess with this unless you know what you're doing.
	 * \param val The new value.
	 */
	virtual void setCurrentVersion(QString val) { setField("JarVersion", val); }
	
	
	/*!
	 * \brief Gets the version of LWJGL that this instance should use.
	 *        If no LWJGL version is specified in the instance's config file, 
	 *        defaults to "Mojang"
	 * \return The instance's LWJGL version.
	 */
	virtual QString lwjglVersion() { return getField("LwjglVersion", "Mojang").value<QString>(); }
	
	/*!
	 * \brief Sets the version of LWJGL that this instance should use.
	 * \param val The LWJGL version to use
	 */
	virtual void setLWJGLVersion(QString val) { setField("LwjglVersion", val); }
	
	
	/*!
	 * \brief Gets the version that this instance should try to update to.
	 *        If this value differs from currentVersion(), the instance will
	 *        download the intended version when it launches.
	 * \return The instance's intended version.
	 */
	virtual QString intendedVersion() { return getField("IntendedJarVersion", currentVersion()).value<QString>(); }
	
	/*!
	 * \brief Sets the version that this instance should try to update to.
	 * \param val The instance's new intended version.
	 */
	virtual void setIntendedVersion(QString val) { setField("IntendedJarVersion", val); }
	
	
	
	//// Timestamps ////
	
	/*!
	 * \brief Gets the time that the instance was last launched.
	 *        Measured in milliseconds since epoch. QDateTime::currentMSecsSinceEpoch()
	 * \return The time that the instance was last launched.
	 */
	virtual qint64 lastLaunch() { return getField("lastLaunchTime", 0).value<qint64>(); }
	
	/*!
	 * \brief Sets the time that the instance was last launched.
	 * \param val The time to set. Defaults to QDateTime::currentMSecsSinceEpoch()
	 */
	virtual void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch()) 
	{ setField("lastLaunchTime", val); }
	
	
	////// Directories //////
	//! Gets the path to the instance's minecraft folder.
	QString minecraftDir() const;
	
	/*!
	 * \brief Gets the path to the instance's instance mods folder.
	 * This is the folder where the jar mods are kept.
	 */
	QString instModsDir() const;
	
	//! Gets the path to the instance's bin folder.
	QString binDir() const;
	
	//! Gets the path to the instance's saves folder.
	QString savesDir() const;
	
	//! Gets the path to the instance's mods folder. (.minecraft/mods)
	QString mlModsDir() const;
	
	//! Gets the path to the instance's coremods folder.
	QString coreModsDir() const;
	
	//! Gets the path to the instance's resources folder.
	QString resourceDir() const;
	
	//! Gets the path to the instance's screenshots folder.
	QString screenshotsDir() const;
	
	//! Gets the path to the instance's texture packs folder.
	QString texturePacksDir() const;
	
	
	////// Files //////
	//! Gets the path to the instance's minecraft.jar
	QString mcJar() const;
	
	//! Gets the path to the instance's mcbackup.jar.
	QString mcBackup() const;
	
	//! Gets the path to the instance's config file.
	QString configFile() const;
	
	//! Gets the path to the instance's modlist file.
	QString modListFile() const;
	
	////// Settings //////
	
	//// Java Settings ////
	DEFINE_OVERRIDDEN_SETTING_ADVANCED(JavaPath, JPATHKEY, QString)
	DEFINE_OVERRIDDEN_SETTING(JvmArgs, QString)
	
	//// Custom Commands ////
	DEFINE_OVERRIDDEN_SETTING(PreLaunchCommand, QString)
	DEFINE_OVERRIDDEN_SETTING(PostExitCommand, QString)
	
	//// Memory ////
	DEFINE_OVERRIDDEN_SETTING(MinMemAlloc, int)
	DEFINE_OVERRIDDEN_SETTING(MaxMemAlloc, int)
	
	//// Auto login ////
	DEFINE_OVERRIDDEN_SETTING(AutoLogin, bool)
	
	// This little guy here is to keep Qt Creator from being a dumbass and 
	// auto-indenting the lines below the macros. Seriously, it's really annoying.
	;
	
	
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
	
protected:
	/*!
	 * \brief Gets the value of the given field in the instance's config file.
	 *        If the value isn't in the config file, defVal is returned instead.
	 * \param name The name of the field in the config file.
	 * \param defVal The default value.
	 * \return The value of the given field or defVal if the field doesn't exist.
	 * \sa setField()
	 */
	virtual QVariant getField(const QString &name, QVariant defVal = QVariant()) const;
	
	/*!
	 * \brief Sets the value of the given field in the config file.
	 * \param name The name of the field in the config file.
	 * \param val The value to set the field to.
	 * \sa getField()
	 */
	virtual void setField(const QString &name, QVariant val);
	
	// Overrides for SettingBase stuff.
	virtual QVariant getValue(const QString &name, QVariant defVal = QVariant()) const
	{ return settings->getValue(name, defVal); }
	virtual void setValue(const QString &name, QVariant val)
	{ setField(name, val); }
	
	INIFile config;
	
private:
	QString m_rootDir;
};

#endif // INSTANCE_H
