#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <memory>
#include "logic/minecraft/OpSys.h"
#include "logic/minecraft/OneSixRule.h"
#include "VersionPatch.h"
#include "MMCError.h"
#include "OneSixLibrary.h"
#include "JarMod.h"

class InstanceVersion;
class VersionFile;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public VersionPatch
{
public: /* methods */
	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename,
								   const bool requireOrder, const bool isFTB = false);
	QJsonDocument toJson(bool saveOrder);

	virtual void applyTo(InstanceVersion *version) override;
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
	virtual bool isCustom()
	{
		return !isVanilla;
	};
	void setVanilla (bool state)
	{
		isVanilla = state;
	}

public: /* data */
	int order = 0;
	bool isVanilla = false;
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
	QString removeMinecraftArguments;
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
	QStringList removeTweakers;

	bool shouldOverwriteLibs = false;
	QList<RawLibraryPtr> overwriteLibs;
	QList<RawLibraryPtr> addLibs;
	QList<QString> removeLibs;

	QSet<QString> traits;

	QList<JarmodPtr> jarMods;
};


