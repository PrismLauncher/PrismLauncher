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
	virtual QString errorName()
	{
		return "VersionBuildError";
	};
	virtual ~VersionBuildError() {};
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
};