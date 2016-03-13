#include "OneSixVersionFormat.h"
#include <minecraft/NullProfileStrategy.h>
#include <Json.h>
#include "minecraft/ParseUtils.h"
#include <minecraft/MinecraftVersion.h>
#include <minecraft/VersionBuildError.h>
#include <minecraft/MojangVersionFormat.h>

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

LibraryPtr OneSixVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
	LibraryPtr out = MojangVersionFormat::libraryFromJson(libObj, filename);
	readString(libObj, "MMC-hint", out->m_hint);
	readString(libObj, "MMC-absulute_url", out->m_absolute_url);
	readString(libObj, "MMC-absoluteUrl", out->m_absolute_url);
	return out;
}

QJsonObject OneSixVersionFormat::libraryToJson(Library *library)
{
	QJsonObject libRoot = MojangVersionFormat::libraryToJson(library);
	if (library->m_absolute_url.size())
		libRoot.insert("MMC-absoluteUrl", library->m_absolute_url);
	if (library->m_hint.size())
		libRoot.insert("MMC-hint", library->m_hint);
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
	readString(root, "minecraftArguments", out->minecraftArguments);
	if(out->minecraftArguments.isEmpty())
	{
		QString processArguments;
		readString(root, "processArguments", processArguments);
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

	readString(root, "type", out->type);

	out->m_releaseTime = timeFromS3Time(readStringRet(root, "releaseTime"));
	out->m_updateTime = timeFromS3Time(readStringRet(root, "time"));

	readString(root, "assets", out->assets);

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

	auto readLibs = [&](const char * which)
	{
		for (auto libVal : requireArray(root.value(which)))
		{
			QJsonObject libObj = requireObject(libVal);
			// parse the library
			auto lib = libraryFromJson(libObj, filename);
			out->libraries.append(lib);
		}
	};
	bool hasPlusLibs = root.contains("+libraries");
	bool hasLibs = root.contains("libraries");
	if (hasPlusLibs && hasLibs)
	{
		out->addProblem(PROBLEM_WARNING, QObject::tr("Version file has both '+libraries' and 'libraries'. This is no longer supported."));
		readLibs("libraries");
		readLibs("+libraries");
	}
	else if (hasLibs)
	{
		readLibs("libraries");
	}
	else if(hasPlusLibs)
	{
		readLibs("+libraries");
	}

	/* removed features that shouldn't be used */
	if (root.contains("tweakers"))
	{
		out->addProblem(PROBLEM_ERROR, QObject::tr("Version file contains unsupported element 'tweakers'"));
	}
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
	if (root.contains("+minecraftArguments"))
	{
		out->addProblem(PROBLEM_ERROR, QObject::tr("Version file contains unsupported element '+minecraftArguments'"));
	}
	return out;
}

template <typename T>
struct libraryConversion
{
	static QJsonObject convert(std::shared_ptr<Library> &value)
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
	writeString(root, "minecraftArguments", patch->minecraftArguments);
	writeString(root, "type", patch->type);
	writeString(root, "assets", patch->assets);
	if (patch->isMinecraftVersion())
	{
		writeString(root, "releaseTime", timeToS3Time(patch->m_releaseTime));
		writeString(root, "time", timeToS3Time(patch->m_updateTime));
	}
	writeStringList(root, "+tweakers", patch->addTweakers);
	writeStringList(root, "+traits", patch->traits.toList());
	if (!patch->libraries.isEmpty())
	{
		QJsonArray array;
		for (auto value: patch->libraries)
		{
			array.append(OneSixVersionFormat::libraryToJson(value.get()));
		}
		root.insert("libraries", array);
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
	if(patch->getVersionSource() == Local && patch->getVersionFile())
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
	throw VersionIncomplete(QObject::tr("Unhandled object type while processing %1").arg(patch->getName()));
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
