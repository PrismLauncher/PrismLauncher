#include "MojangVersionFormat.h"
#include "onesix/OneSixVersionFormat.h"
#include "MinecraftVersion.h"
#include "VersionBuildError.h"
#include "MojangDownloadInfo.h"

#include "Json.h"
using namespace Json;
#include "ParseUtils.h"

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 14;

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

	out->name = "Minecraft";
	out->fileId = "net.minecraft";
	out->version = root.value("version").toString();
	out->filename = filename;

	Bits::readString(root, "id", out->id);

	Bits::readString(root, "mainClass", out->mainClass);
	Bits::readString(root, "minecraftArguments", out->overwriteMinecraftArguments);
	Bits::readString(root, "type", out->type);

	if(root.contains("assetIndex"))
	{
		out->mojangAssetIndex = assetIndexFromJson(requireObject(root, "assetIndex"));
	}
	Bits::readString(root, "assets", out->assets);

	out->m_releaseTime = timeFromS3Time(root.value("releaseTime").toString(""));
	out->m_updateTime = timeFromS3Time(root.value("time").toString(""));

	if (root.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = requireInteger(root.value("minimumLauncherVersion"));
		if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			out->addProblem(
				PROBLEM_WARNING,
				QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
					.arg(out->minimumLauncherVersion)
					.arg(CURRENT_MINIMUM_LAUNCHER_VERSION));
		}
	}

	if (root.contains("libraries"))
	{
		for (auto libVal : requireArray(root.value("libraries")))
		{
			auto libObj = requireObject(libVal);

			auto lib = MojangVersionFormat::libraryFromJson(libObj, filename);
			out->addLibs.append(lib);
		}
	}
	if(root.contains("downloads"))
	{
		auto downloadsObj = requireObject(root, "downloads");
		for(auto iter = downloadsObj.begin(); iter != downloadsObj.end(); iter++)
		{
			auto classifier = iter.key();
			auto classifierObj = requireObject(iter.value());
			out->mojangDownloads[classifier] = downloadInfoFromJson(classifierObj);
		}
	}
	return out;
}

QJsonDocument versionFileToJson(VersionFilePtr patch)
{
	QJsonObject root;
	writeString(root, "id", patch->id);
	writeString(root, "mainClass", patch->mainClass);
	writeString(root, "processArguments", patch->processArguments);
	writeString(root, "minecraftArguments", patch->overwriteMinecraftArguments);
	writeString(root, "type", patch->type);
	writeString(root, "assets", patch->assets);
	writeString(root, "releaseTime", timeToS3Time(patch->m_releaseTime));
	writeString(root, "time", timeToS3Time(patch->m_updateTime));
	if(patch->minimumLauncherVersion != -1)
	{
		root.insert("minimumLauncherVersion", patch->minimumLauncherVersion);
	}

	if (!patch->addLibs.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->addLibs)
		{
			array.append(MojangVersionFormat::libraryToJson(value.get()));
		}
		root.insert("libraries", array);
	}
	if(patch->mojangAssetIndex && patch->mojangAssetIndex->known)
	{
		root.insert("assetIndex", assetIndexToJson(patch->mojangAssetIndex));
	}
	if(patch->mojangDownloads.size())
	{
		QJsonObject downloadsOut;
		for(auto iter = patch->mojangDownloads.begin(); iter != patch->mojangDownloads.end(); iter++)
		{
			downloadsOut.insert(iter.key(), downloadInfoToJson(iter.value()));
		}
		root.insert("downloads", downloadsOut);
	}
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

static QJsonDocument minecraftVersionToJson(MinecraftVersionPtr patch)
{
	if(patch->m_versionSource == Local && patch->getVersionFile())
	{
		return MojangVersionFormat::profilePatchToJson(patch->getVersionFile());
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't write incomplete/builtin Minecraft version %1").arg(patch->name()));
	}
}

QJsonDocument MojangVersionFormat::profilePatchToJson(const ProfilePatchPtr &patch)
{
	auto vfile = std::dynamic_pointer_cast<VersionFile>(patch);
	if(vfile)
	{
		return versionFileToJson(vfile);
	}
	auto mversion = std::dynamic_pointer_cast<MinecraftVersion>(patch);
	if(mversion)
	{
		return minecraftVersionToJson(mversion);
	}
	throw VersionIncomplete(QObject::tr("Unhandled object type while processing %1").arg(patch->getPatchName()));
}

LibraryPtr MojangVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
	LibraryPtr out(new Library());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename + "contains a library that doesn't have a 'name' field");
	}
	out->m_name = libObj.value("name").toString();

	Bits::readString(libObj, "url", out->m_base_url);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = requireObject(libObj.value("extract"));
		for (auto excludeVal : requireArray(extractObj.value("exclude")))
		{
			out->extract_excludes.append(requireString(excludeVal));
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
				out->m_native_classifiers[opSys] = it.value().toString();
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
		out->m_mojang_downloads = libDownloadInfoFromJson(libObj);
	}
	return out;
}

QJsonObject MojangVersionFormat::libraryToJson(Library *library)
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)library->m_name);
	if (library->m_base_url != "http://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		library->m_base_url != "https://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		library->m_base_url != "https://" + URLConstants::LIBRARY_BASE && !library->m_base_url.isEmpty())
	{
		libRoot.insert("url", library->m_base_url);
	}
	if (library->isNative())
	{
		QJsonObject nativeList;
		auto iter = library->m_native_classifiers.begin();
		while (iter != library->m_native_classifiers.end())
		{
			nativeList.insert(OpSys_toString(iter.key()), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
		if (library->extract_excludes.size())
		{
			QJsonArray excludes;
			QJsonObject extract;
			for (auto exclude : library->extract_excludes)
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
	if(library->m_mojang_downloads)
	{
		auto downloadsObj = libDownloadInfoToJson(library->m_mojang_downloads);
		libRoot.insert("downloads", downloadsObj);
	}
	return libRoot;
}
