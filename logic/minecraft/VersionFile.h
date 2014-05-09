#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include "logic/minecraft/OpSys.h"
#include "logic/minecraft/OneSixRule.h"
#include "VersionPatch.h"
#include "MMCError.h"
#include "RawLibrary.h"
#include "JarMod.h"

class VersionFinal;


class VersionBuildError : public MMCError
{
public:
	VersionBuildError(QString cause) : MMCError(cause) {};
	virtual ~VersionBuildError() noexcept {}
};

/**
 * the base version file was meant for a newer version of the vanilla launcher than we support
 */
class LauncherVersionError : public VersionBuildError
{
public:
	LauncherVersionError(int actual, int supported)
		: VersionBuildError(QObject::tr(
			  "The base version file of this instance was meant for a newer (%1) "
			  "version of the vanilla launcher than this version of MultiMC supports (%2).")
								.arg(actual)
								.arg(supported)) {};
	virtual ~LauncherVersionError() noexcept {}
};

/**
 * some patch was intended for a different version of minecraft
 */
class MinecraftVersionMismatch : public VersionBuildError
{
public:
	MinecraftVersionMismatch(QString fileId, QString mcVersion, QString parentMcVersion)
		: VersionBuildError(QObject::tr("The patch %1 is for a different version of Minecraft "
										"(%2) than that of the instance (%3).")
								.arg(fileId)
								.arg(mcVersion)
								.arg(parentMcVersion)) {};
	virtual ~MinecraftVersionMismatch() noexcept {}
};

struct VersionFile;
typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public VersionPatch
{
public: /* methods */
	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename,
								   const bool requireOrder, const bool isFTB = false);

	static OneSixLibraryPtr createLibrary(RawLibraryPtr lib);
	virtual void applyTo(VersionFinal *version) override;
	virtual bool isVanilla() override;
	virtual bool hasJarMods() override;
	
public: /* data */
	int order = 0;
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
	QString versionReleaseTime;
	QString versionFileUpdateTime;
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


