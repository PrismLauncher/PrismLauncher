#include "MojangVersionFormat.h"
#include "onesix/OneSixVersionFormat.h"
#include "MinecraftVersion.h"
#include "VersionBuildError.h"
#include "MojangDownloadInfo.h"

#include "Json.h"
using namespace Json;
#include "ParseUtils.h"

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 18;

static MojangAssetIndexInfo::Ptr assetIndexFromJson (const QJsonObject &obj);
static MojangDownloadInfo::Ptr downloadInfoFromJson (const QJsonObject &obj);
static MojangLibraryDownloadInfo::Ptr libDownloadInfoFromJson (const QJsonObject &libObj);
static QJsonObject assetIndexToJson (MojangAssetIndexInfo::Ptr assetidxinfo);
static QJsonObject libDownloadInfoToJson (MojangLibraryDownloadInfo::Ptr libinfo);
static QJsonObject downloadInfoToJson (MojangDownloadInfo::Ptr info);

namespace Bits
{
static void readString(const QJsonObject &root, const QString &key, QString &variable)
{
	if (root.contains(key))
	{
		variable = requireString(root.value(key));
	}
}

static void readDownloadInfo(MojangDownloadInfo::Ptr out, const QJsonObject &obj)
{
	// optional, not used
	readString(obj, "path", out->path);
	// required!
	out->sha1 = requireString(obj, "sha1");
	out->url = requireString(obj, "url");
	out->size = requireInteger(obj, "size");
}

static void readAssetIndex(MojangAssetIndexInfo::Ptr out, const QJsonObject &obj)
{
	out->totalSize = requireInteger(obj, "totalSize");
	out->id = requireString(obj, "id");
	// out->known = true;
}
}

MojangDownloadInfo::Ptr downloadInfoFromJson(const QJsonObject &obj)
{
	auto out = std::make_shared<MojangDownloadInfo>();
	Bits::readDownloadInfo(out, obj);
	return out;
}

MojangAssetIndexInfo::Ptr assetIndexFromJson(const QJsonObject &obj)
{
	auto out = std::make_shared<MojangAssetIndexInfo>();
	Bits::readDownloadInfo(out, obj);
	Bits::readAssetIndex(out, obj);
	return out;
}

QJsonObject downloadInfoToJson(MojangDownloadInfo::Ptr info)
{
	QJsonObject out;
	if(!info->path.isNull())
	{
		out.insert("path", info->path);
	}
	out.insert("sha1", info->sha1);
	out.insert("size", info->size);
	out.insert("url", info->url);
	return out;
}

MojangLibraryDownloadInfo::Ptr libDownloadInfoFromJson(const QJsonObject &libObj)
{
	auto out = std::make_shared<MojangLibraryDownloadInfo>();
	auto dlObj = requireObject(libObj.value("downloads"));
	if(dlObj.contains("artifact"))
	{
		out->artifact = downloadInfoFromJson(requireObject(dlObj, "artifact"));
	}
	if(dlObj.contains("classifiers"))
	{
		auto classifiersObj = requireObject(dlObj, "classifiers");
		for(auto iter = classifiersObj.begin(); iter != classifiersObj.end(); iter++)
		{
			auto classifier = iter.key();
			auto classifierObj = requireObject(iter.value());
			out->classifiers[classifier] = downloadInfoFromJson(classifierObj);
		}
	}
	return out;
}

QJsonObject libDownloadInfoToJson(MojangLibraryDownloadInfo::Ptr libinfo)
{
	QJsonObject out;
	if(libinfo->artifact)
	{
		out.insert("artifact", downloadInfoToJson(libinfo->artifact));
	}
	if(libinfo->classifiers.size())
	{
		QJsonObject classifiersOut;
		for(auto iter = libinfo->classifiers.begin(); iter != libinfo->classifiers.end(); iter++)
		{
			classifiersOut.insert(iter.key(), downloadInfoToJson(iter.value()));
		}
		out.insert("classifiers", classifiersOut);
	}
	return out;
}

QJsonObject assetIndexToJson(MojangAssetIndexInfo::Ptr info)
{
	QJsonObject out;
	if(!info->path.isNull())
	{
		out.insert("path", info->path);
	}
	out.insert("sha1", info->sha1);
	out.insert("size", info->size);
	out.insert("url", info->url);
	out.insert("totalSize", info->totalSize);
	out.insert("id", info->id);
	return out;
}

void MojangVersionFormat::readVersionProperties(const QJsonObject &in, VersionFile *out)
{
	Bits::readString(in, "id", out->minecraftVersion);
	Bits::readString(in, "mainClass", out->mainClass);
	Bits::readString(in, "minecraftArguments", out->minecraftArguments);
	if(out->minecraftArguments.isEmpty())
	{
		QString processArguments;
		Bits::readString(in, "processArguments", processArguments);
		QString toCompare = processArguments.toLower();
		if (toCompare == "legacy")
		{
			out->minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			out->minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			out->minecraftArguments = "--username ${auth_player_name} --session ${auth_session} --version ${profile_name}";
		}
		else if (!toCompare.isEmpty())
		{
			out->addProblem(PROBLEM_ERROR, QObject::tr("processArguments is set to unknown value '%1'").arg(processArguments));
		}
	}
	Bits::readString(in, "type", out->type);

	Bits::readString(in, "assets", out->assets);
	if(in.contains("assetIndex"))
	{
		out->mojangAssetIndex = assetIndexFromJson(requireObject(in, "assetIndex"));
	}
	else if (!out->assets.isNull())
	{
		out->mojangAssetIndex = std::make_shared<MojangAssetIndexInfo>(out->assets);
	}

	out->m_releaseTime = timeFromS3Time(in.value("releaseTime").toString(""));
	out->m_updateTime = timeFromS3Time(in.value("time").toString(""));

	if (in.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = requireInteger(in.value("minimumLauncherVersion"));
		if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			out->addProblem(
				PROBLEM_WARNING,
				QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
					.arg(out->minimumLauncherVersion)
					.arg(CURRENT_MINIMUM_LAUNCHER_VERSION));
		}
	}
	if(in.contains("downloads"))
	{
		auto downloadsObj = requireObject(in, "downloads");
		for(auto iter = downloadsObj.begin(); iter != downloadsObj.end(); iter++)
		{
			auto classifier = iter.key();
			auto classifierObj = requireObject(iter.value());
			out->mojangDownloads[classifier] = downloadInfoFromJson(classifierObj);
		}
	}
}

VersionFilePtr MojangVersionFormat::versionFileFromJson(const QJsonDocument &doc, const QString &filename)
{
	VersionFilePtr out(new VersionFile());
	if (doc.isEmpty() || doc.isNull())
	{
		throw JSONValidationError(filename + " is empty or null");
	}
	if (!doc.isObject())
	{
		throw JSONValidationError(filename + " is not an object");
	}

	QJsonObject root = doc.object();

	readVersionProperties(root, out.get());

	out->name = "Minecraft";
	out->fileId = "net.minecraft";
	out->version = out->minecraftVersion;
	out->filename = filename;


	if (root.contains("libraries"))
	{
		for (auto libVal : requireArray(root.value("libraries")))
		{
			auto libObj = requireObject(libVal);

			auto lib = MojangVersionFormat::libraryFromJson(libObj, filename);
			out->libraries.append(lib);
		}
	}
	return out;
}

void MojangVersionFormat::writeVersionProperties(const VersionFile* in, QJsonObject& out)
{
	writeString(out, "id", in->minecraftVersion);
	writeString(out, "mainClass", in->mainClass);
	writeString(out, "minecraftArguments", in->minecraftArguments);
	writeString(out, "type", in->type);
	if(!in->m_releaseTime.isNull())
	{
		writeString(out, "releaseTime", timeToS3Time(in->m_releaseTime));
	}
	if(!in->m_updateTime.isNull())
	{
		writeString(out, "time", timeToS3Time(in->m_updateTime));
	}
	if(in->minimumLauncherVersion != -1)
	{
		out.insert("minimumLauncherVersion", in->minimumLauncherVersion);
	}
	writeString(out, "assets", in->assets);
	if(in->mojangAssetIndex && in->mojangAssetIndex->known)
	{
		out.insert("assetIndex", assetIndexToJson(in->mojangAssetIndex));
	}
	if(in->mojangDownloads.size())
	{
		QJsonObject downloadsOut;
		for(auto iter = in->mojangDownloads.begin(); iter != in->mojangDownloads.end(); iter++)
		{
			downloadsOut.insert(iter.key(), downloadInfoToJson(iter.value()));
		}
		out.insert("downloads", downloadsOut);
	}
}

QJsonDocument MojangVersionFormat::versionFileToJson(const VersionFilePtr &patch)
{
	QJsonObject root;
	writeVersionProperties(patch.get(), root);
	if (!patch->libraries.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->libraries)
		{
			array.append(MojangVersionFormat::libraryToJson(value.get()));
		}
		root.insert("libraries", array);
	}

	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

LibraryPtr MojangVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
	LibraryPtr out(new Library());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename + "contains a library that doesn't have a 'name' field");
	}
	out->m_name = libObj.value("name").toString();

	Bits::readString(libObj, "url", out->m_repositoryURL);
	if (libObj.contains("extract"))
	{
		out->m_hasExcludes = true;
		auto extractObj = requireObject(libObj.value("extract"));
		for (auto excludeVal : requireArray(extractObj.value("exclude")))
		{
			out->m_extractExcludes.append(requireString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		QJsonObject nativesObj = requireObject(libObj.value("natives"));
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				qWarning() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys_fromString(it.key());
			if (opSys != Os_Other)
			{
				out->m_nativeClassifiers[opSys] = it.value().toString();
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->m_rules = rulesFromJsonV4(libObj);
	}
	if (libObj.contains("downloads"))
	{
		out->m_mojangDownloads = libDownloadInfoFromJson(libObj);
	}
	return out;
}

QJsonObject MojangVersionFormat::libraryToJson(Library *library)
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)library->m_name);
	if (!library->m_repositoryURL.isEmpty())
	{
		libRoot.insert("url", library->m_repositoryURL);
	}
	if (library->isNative())
	{
		QJsonObject nativeList;
		auto iter = library->m_nativeClassifiers.begin();
		while (iter != library->m_nativeClassifiers.end())
		{
			nativeList.insert(OpSys_toString(iter.key()), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
		if (library->m_extractExcludes.size())
		{
			QJsonArray excludes;
			QJsonObject extract;
			for (auto exclude : library->m_extractExcludes)
			{
				excludes.append(exclude);
			}
			extract.insert("exclude", excludes);
			libRoot.insert("extract", extract);
		}
	}
	if (library->m_rules.size())
	{
		QJsonArray allRules;
		for (auto &rule : library->m_rules)
		{
			QJsonObject ruleObj = rule->toJson();
			allRules.append(ruleObj);
		}
		libRoot.insert("rules", allRules);
	}
	if(library->m_mojangDownloads)
	{
		auto downloadsObj = libDownloadInfoToJson(library->m_mojangDownloads);
		libRoot.insert("downloads", downloadsObj);
	}
	return libRoot;
}
