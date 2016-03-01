#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/OneSixRule.h"
#include "ProfilePatch.h"
#include "RawLibrary.h"
#include "JarMod.h"

class MinecraftProfile;
class VersionFile;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public ProfilePatch
{
public: /* methods */
	virtual void applyTo(MinecraftProfile *version) override;
	virtual bool isMinecraftVersion() override;
	virtual bool hasJarMods() override;
	virtual int getOrder() override
	{
		return order;
	}
	virtual void setOrder(int order) override
	{
		this->order = order;
	}
	virtual QList<JarmodPtr> getJarMods() override
	{
		return jarMods;
	}
	virtual QString getPatchID() override
	{
		return fileId;
	}
	virtual QString getPatchName() override
	{
		return name;
	}
	virtual QString getPatchVersion() override
	{
		return version;
	}
	virtual QString getPatchFilename() override
	{
		return filename;
	}
	virtual bool isCustom() override
	{
		return !m_isVanilla;
	};
	virtual bool isCustomizable() override
	{
		return m_isCustomizable;
	}
	virtual bool isRemovable() override
	{
		return m_isRemovable;
	}
	virtual bool isRevertible() override
	{
		return m_isRevertible;
	}
    virtual bool isMoveable() override
	{
		return m_isMovable;
	}
    virtual bool isEditable() override
	{
		return isCustom();
	}
	virtual bool isVersionChangeable() override
	{
		return false;
	}

	void setVanilla (bool state)
	{
		m_isVanilla = state;
	}
	void setRemovable (bool state)
	{
		m_isRemovable = state;
	}
	void setRevertible (bool state)
	{
		m_isRevertible = state;
	}
	void setCustomizable (bool state)
	{
		m_isCustomizable = state;
	}
	void setMovable (bool state)
	{
		m_isMovable = state;
	}


public: /* data */
	/// MultiMC: order hint for this version file if no explicit order is set
	int order = 0;

	// Flags for UI and version file manipulation in general
	bool m_isVanilla = false;
	bool m_isRemovable = false;
	bool m_isRevertible = false;
	bool m_isCustomizable = false;
	bool m_isMovable = false;

	/// MultiMC: filename of the file this was loaded from
	QString filename;

	/// MultiMC: human readable name of this package
	QString name;

	/// MultiMC: package ID of this package
	QString fileId;

	/// MultiMC: version of this package
	QString version;

	/// MultiMC: dependency on a Minecraft version
	QString mcVersion;

	/// Mojang: used to version the Mojang version format
	int minimumLauncherVersion = -1;

	/// Mojang: version of Minecraft this is
	QString id;

	/// Mojang: class to launch Minecraft with
	QString mainClass;

	/// MultiMC: class to launch legacy Minecraft with (ambed in a custom window)
	QString appletClass;

	/// Mojang: Minecraft launch arguments (may contain placeholders for variable substitution)
	QString overwriteMinecraftArguments;

	/// MultiMC: Minecraft launch arguments, additive variant
	QString addMinecraftArguments;

	/// Mojang: DEPRECATED variant of the Minecraft arguments, hardcoded, do not use!
	QString processArguments;

	/// Mojang: type of the Minecraft version
	QString type;

	/// Mojang: the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// Mojang: the time this version was last updated by Mojang, as string and as QDateTime
	QString m_updateTimeString;
	QDateTime m_updateTime;

	/// Mojang: DEPRECATED asset group to be used with Minecraft
	QString assets;

	/// MultiMC: override list of tweaker mod arguments for launchwrapper (replaces the previously assembled lists)
	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;

	/// MultiMC: list of tweaker mod arguments for launchwrapper
	QStringList addTweakers;

	/// MultiMC: override list of libraries (replaces the previously assembled lists)
	bool shouldOverwriteLibs = false;
	QList<RawLibraryPtr> overwriteLibs;

	/// Mojang: list of libraries to add to the version
	QList<RawLibraryPtr> addLibs;

	/// MultiMC: list of attached traits of this version file - used to enable features
	QSet<QString> traits;

	/// MultiMC: list of jar mods added to this version
	QList<JarmodPtr> jarMods;
};


