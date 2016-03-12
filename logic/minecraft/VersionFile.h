#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/Rule.h"
#include "ProfilePatch.h"
#include "Library.h"
#include "JarMod.h"

class MinecraftProfile;
class VersionFile;
struct MojangDownloadInfo;
struct MojangAssetIndexInfo;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public ProfilePatch
{
	friend class MojangVersionFormat;
	friend class OneSixVersionFormat;
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
	virtual QString getID() override
	{
		return fileId;
	}
	virtual QString getName() override
	{
		return name;
	}
	virtual QString getVersion() override
	{
		return version;
	}
	virtual QString getFilename() override
	{
		return filename;
	}
	virtual QDateTime getReleaseDateTime() override
	{
		return m_releaseTime;
	}
	VersionSource getVersionSource() override
	{
		return Local;
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

	/// MultiMC: DEPRECATED class to launch legacy Minecraft with (ambed in a custom window)
	QString appletClass;

	/// Mojang: Minecraft launch arguments (may contain placeholders for variable substitution)
	QString minecraftArguments;

	/// Mojang: type of the Minecraft version
	QString type;

	/// Mojang: the time this version was actually released by Mojang
	QDateTime m_releaseTime;

	/// Mojang: the time this version was last updated by Mojang
	QDateTime m_updateTime;

	/// Mojang: DEPRECATED asset group to be used with Minecraft
	QString assets;

	/// MultiMC: list of tweaker mod arguments for launchwrapper
	QStringList addTweakers;

	/// Mojang: list of libraries to add to the version
	QList<LibraryPtr> addLibs;

	/// MultiMC: list of attached traits of this version file - used to enable features
	QSet<QString> traits;

	/// MultiMC: list of jar mods added to this version
	QList<JarmodPtr> jarMods;

public:
	// Mojang: list of 'downloads' - client jar, server jar, windows server exe, maybe more.
	QMap <QString, std::shared_ptr<MojangDownloadInfo>> mojangDownloads;

	// Mojang: extended asset index download information
	std::shared_ptr<MojangAssetIndexInfo> mojangAssetIndex;
};


