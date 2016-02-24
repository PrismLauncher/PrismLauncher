#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/OneSixRule.h"
#include "ProfilePatch.h"
#include "OneSixLibrary.h"
#include "JarMod.h"

class MinecraftProfile;
class VersionFile;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public ProfilePatch
{
public: /* methods */
	static VersionFilePtr fromMojangJson(const QJsonDocument &doc, const QString &filename);
	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename,
								   const bool requireOrder);
	virtual QJsonDocument toJson(bool saveOrder) override;

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
	int order = 0;
	bool m_isVanilla = false;
	bool m_isRemovable = false;
	bool m_isRevertible = false;
	bool m_isCustomizable = false;
	bool m_isMovable = false;
	QString name;
	QString fileId;
	QString version;
	// TODO use the mcVersion to determine if a version file should be removed on update
	QString mcVersion;
	QString filename;
	// TODO requirements
	// QMap<QString, QString> requirements;
	QString id;
	QString mainClass;
	QString appletClass;
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString processArguments;
	QString type;

	/// the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// the time this version was last updated by Mojang, as string and as QDateTime
	QString m_updateTimeString;
	QDateTime m_updateTime;

	/// asset group used by this ... thing.
	QString assets;
	int minimumLauncherVersion = -1;

	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;
	QStringList addTweakers;

	bool shouldOverwriteLibs = false;
	QList<RawLibraryPtr> overwriteLibs;
	QList<RawLibraryPtr> addLibs;

	QSet<QString> traits;

	QList<JarmodPtr> jarMods;
};


