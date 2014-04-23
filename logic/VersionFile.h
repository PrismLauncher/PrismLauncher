#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include "logic/OpSys.h"
#include "logic/OneSixRule.h"
#include "MMCError.h"

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

struct RawLibrary;
typedef std::shared_ptr<RawLibrary> RawLibraryPtr;
struct RawLibrary
{
	QString name;
	QString url;
	QString hint;
	QString absoluteUrl;
	bool applyExcludes = false;
	QStringList excludes;
	bool applyNatives = false;
	QList<QPair<OpSys, QString>> natives;
	bool applyRules = false;
	QList<std::shared_ptr<Rule>> rules;

	// user for '+' libraries
	enum InsertType
	{
		Apply,
		Append,
		Prepend,
		Replace
	};
	InsertType insertType = Append;
	QString insertData;
	enum DependType
	{
		Soft,
		Hard
	};
	DependType dependType = Soft;

	static RawLibraryPtr fromJson(const QJsonObject &libObj, const QString &filename);
};

struct Jarmod;
typedef std::shared_ptr<Jarmod> JarmodPtr;
struct Jarmod
{
	QString name;
	QString baseurl;
	QString hint;
	QString absoluteUrl;
	
	static JarmodPtr fromJson(const QJsonObject &libObj, const QString &filename);
};

struct VersionFile;
typedef std::shared_ptr<VersionFile> VersionFilePtr;
struct VersionFile
{
public: /* methods */
	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename,
								   const bool requireOrder, const bool isFTB = false);

	static OneSixLibraryPtr createLibrary(RawLibraryPtr lib);
	int findLibrary(QList<OneSixLibraryPtr> haystack, const QString &needle);
	void applyTo(VersionFinal *version);
	bool isVanilla();
	bool hasJarMods();
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
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString removeMinecraftArguments;
	QString processArguments;
	QString type;
	QString releaseTime;
	QString time;
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
