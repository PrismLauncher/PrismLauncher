#include "OneSixVersionFormat.h"
#include <minecraft/NullProfileStrategy.h>
#include <Json.h>
#include "minecraft/ParseUtils.h"
#include <minecraft/MinecraftVersion.h>
#include <minecraft/VersionBuildError.h>

using namespace Json;

static void readString(const QJsonObject &root, const QString &key, QString &variable)
{
	if (root.contains(key))
	{
		variable = requireString(root.value(key));
	}
}

static QString readStringRet(const QJsonObject &root, const QString &key)
{
	if (root.contains(key))
	{
		return requireString(root.value(key));
	}
	return QString();
}

RawLibraryPtr OneSixVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
	RawLibraryPtr out(new RawLibrary());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a library that doesn't have a 'name' field");
	}
	out->m_name = libObj.value("name").toString();

	readString(libObj, "url", out->m_base_url);
	readString(libObj, "MMC-hint", out->m_hint);
	readString(libObj, "MMC-absulute_url", out->m_absolute_url);
	readString(libObj, "MMC-absoluteUrl", out->m_absolute_url);
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
	return out;
}

QJsonObject OneSixVersionFormat::libraryToJson(RawLibrary *library)
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)library->m_name);
	if (library->m_absolute_url.size())
		libRoot.insert("MMC-absoluteUrl", library->m_absolute_url);
	if (library->m_hint.size())
		libRoot.insert("MMC-hint", library->m_hint);
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
	return libRoot;
}

VersionFilePtr OneSixVersionFormat::versionFileFromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder)
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

	if (requireOrder)
	{
		if (root.contains("order"))
		{
			out->order = requireInteger(root.value("order"));
		}
		else
		{
			// FIXME: evaluate if we don't want to throw exceptions here instead
			qCritical() << filename << "doesn't contain an order field";
		}
	}

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	out->mcVersion = root.value("mcVersion").toString();
	out->filename = filename;

	readString(root, "id", out->id);

	readString(root, "mainClass", out->mainClass);
	readString(root, "appletClass", out->appletClass);
	readString(root, "processArguments", out->processArguments);
	readString(root, "minecraftArguments", out->overwriteMinecraftArguments);
	readString(root, "+minecraftArguments", out->addMinecraftArguments);
	readString(root, "type", out->type);

	out->m_releaseTime = timeFromS3Time(readStringRet(root, "releaseTime"));
	out->m_updateTime = timeFromS3Time(readStringRet(root, "time"));

	readString(root, "assets", out->assets);

	if (root.contains("tweakers"))
	{
		out->shouldOverwriteTweakers = true;
		for (auto tweakerVal : requireArray(root.value("tweakers")))
		{
			out->overwriteTweakers.append(requireString(tweakerVal));
		}
	}

	if (root.contains("+tweakers"))
	{
		for (auto tweakerVal : requireArray(root.value("+tweakers")))
		{
			out->addTweakers.append(requireString(tweakerVal));
		}
	}


	if (root.contains("+traits"))
	{
		for (auto tweakerVal : requireArray(root.value("+traits")))
		{
			out->traits.insert(requireString(tweakerVal));
		}
	}

	if (root.contains("libraries"))
	{
		out->shouldOverwriteLibs = true;
		for (auto libVal : requireArray(root.value("libraries")))
		{
			auto libObj = requireObject(libVal);

			auto lib = libraryFromJson(libObj, filename);
			out->overwriteLibs.append(lib);
		}
	}

	if (root.contains("+jarMods"))
	{
		for (auto libVal : requireArray(root.value("+jarMods")))
		{
			QJsonObject libObj = requireObject(libVal);
			// parse the jarmod
			auto lib = OneSixVersionFormat::jarModFromJson(libObj, filename, out->name);
			if(lib->originalName.isEmpty())
			{
				auto fixed = out->name;
				fixed.remove(" (jar mod)");
				lib->originalName = out->name;
			}
			// and add to jar mods
			out->jarMods.append(lib);
		}
	}

	if (root.contains("+libraries"))
	{
		for (auto libVal : requireArray(root.value("+libraries")))
		{
			QJsonObject libObj = requireObject(libVal);
			// parse the library
			auto lib = libraryFromJson(libObj, filename);
			out->addLibs.append(lib);
		}
	}

	/* removed features that shouldn't be used */
	if (root.contains("-libraries"))
	{
		out->addProblem(PROBLEM_ERROR, QObject::tr("Version file contains unsupported element '-libraries'"));
	}
	if (root.contains("-tweakers"))
	{
		out->addProblem(PROBLEM_ERROR, QObject::tr("Version file contains unsupported element '-tweakers'"));
	}
	if (root.contains("-minecraftArguments"))
	{
		out->addProblem(PROBLEM_ERROR, QObject::tr("Version file contains unsupported element '-minecraftArguments'"));
	}
	return out;
}

template <typename T>
struct libraryConversion
{
	static QJsonObject convert(std::shared_ptr<RawLibrary> &value)
	{
		return OneSixVersionFormat::libraryToJson(value.get());
	}
};

static QJsonDocument versionFileToJson(VersionFilePtr patch, bool saveOrder)
{
	QJsonObject root;
	if (saveOrder)
	{
		root.insert("order", patch->order);
	}
	writeString(root, "name", patch->name);
	writeString(root, "fileId", patch->fileId);
	writeString(root, "version", patch->version);
	writeString(root, "mcVersion", patch->mcVersion);
	writeString(root, "id", patch->id);
	writeString(root, "mainClass", patch->mainClass);
	writeString(root, "appletClass", patch->appletClass);
	writeString(root, "processArguments", patch->processArguments);
	writeString(root, "minecraftArguments", patch->overwriteMinecraftArguments);
	writeString(root, "+minecraftArguments", patch->addMinecraftArguments);
	writeString(root, "type", patch->type);
	writeString(root, "assets", patch->assets);
	if (patch->isMinecraftVersion())
	{
		writeString(root, "releaseTime", timeToS3Time(patch->m_releaseTime));
		writeString(root, "time", timeToS3Time(patch->m_updateTime));
	}
	writeStringList(root, "tweakers", patch->overwriteTweakers);
	writeStringList(root, "+tweakers", patch->addTweakers);
	writeStringList(root, "+traits", patch->traits.toList());
	if (!patch->overwriteLibs.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->overwriteLibs)
		{
			array.append(OneSixVersionFormat::libraryToJson(value.get()));
		}
		root.insert("libraries", array);
	}
	if (!patch->addLibs.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->addLibs)
		{
			array.append(OneSixVersionFormat::libraryToJson(value.get()));
		}
		root.insert("+libraries", array);
	}
	if (!patch->jarMods.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->jarMods)
		{
			array.append(OneSixVersionFormat::jarModtoJson(value.get()));
		}
		root.insert("+jarMods", array);
	}
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

static QJsonDocument minecraftVersionToJson(MinecraftVersionPtr patch, bool saveOrder)
{
	if(patch->m_versionSource == Local && patch->getVersionFile())
	{
		return OneSixVersionFormat::profilePatchToJson(patch->getVersionFile(), saveOrder);
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't write incomplete/builtin Minecraft version %1").arg(patch->name()));
	}
}

QJsonDocument OneSixVersionFormat::profilePatchToJson(const ProfilePatchPtr &patch, bool saveOrder)
{
	auto vfile = std::dynamic_pointer_cast<VersionFile>(patch);
	if(vfile)
	{
		return versionFileToJson(vfile, saveOrder);
	}
	auto mversion = std::dynamic_pointer_cast<MinecraftVersion>(patch);
	if(mversion)
	{
		return minecraftVersionToJson(mversion, saveOrder);
	}
	throw VersionIncomplete(QObject::tr("Unhandled object type while processing %1").arg(patch->getPatchName()));
}

std::shared_ptr<MinecraftProfile> OneSixVersionFormat::profileFromSingleJson(const QJsonObject &obj)
{
	std::shared_ptr<MinecraftProfile> version(new MinecraftProfile(new NullProfileStrategy()));
	try
	{
		version->clear();
		auto file = versionFileFromJson(QJsonDocument(obj), QString(), false);
		file->applyTo(version.get());
		version->appendPatch(file);
	}
	catch(Exception &err)
	{
		return nullptr;
	}
	return version;
}

JarmodPtr OneSixVersionFormat::jarModFromJson(const QJsonObject &libObj, const QString &filename, const QString &originalName)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();
	out->originalName = libObj.value("originalName").toString();
	return out;
}

QJsonObject OneSixVersionFormat::jarModtoJson(Jarmod *jarmod)
{
	QJsonObject out;
	writeString(out, "name", jarmod->name);
	if(!jarmod->originalName.isEmpty())
	{
		writeString(out, "originalName", jarmod->originalName);
	}
	return out;
}
