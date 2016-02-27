#include "OneSixVersionFormat.h"
#include <minecraft/NullProfileStrategy.h>
#include <Json.h>
#include "minecraft/ParseUtils.h"

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

VersionFilePtr OneSixVersionFormat::fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder)
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

	parse_timestamp(readStringRet(root, "releaseTime"), out->m_releaseTimeString, out->m_releaseTime);
	parse_timestamp(readStringRet(root, "time"), out->m_updateTimeString, out->m_updateTime);

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

			auto lib = RawLibrary::fromJson(libObj, filename);
			out->overwriteLibs.append(lib);
		}
	}

	if (root.contains("+jarMods"))
	{
		for (auto libVal : requireArray(root.value("+jarMods")))
		{
			QJsonObject libObj = requireObject(libVal);
			// parse the jarmod
			auto lib = Jarmod::fromJson(libObj, filename, out->name);
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
			auto lib = RawLibrary::fromJson(libObj, filename);
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
		writeString(root, "releaseTime", patch->m_releaseTimeString);
		writeString(root, "time", patch->m_updateTimeString);
	}
	writeStringList(root, "tweakers", patch->overwriteTweakers);
	writeStringList(root, "+tweakers", patch->addTweakers);
	writeStringList(root, "+traits", patch->traits.toList());
	writeObjectList(root, "libraries", patch->overwriteLibs);
	writeObjectList(root, "+libraries", patch->addLibs);
	writeObjectList(root, "+jarMods", patch->jarMods);
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

QJsonDocument OneSixVersionFormat::toJson(const ProfilePatchPtr &patch, bool saveOrder)
{
	auto vfile = std::dynamic_pointer_cast<VersionFile>(patch);
	if(vfile)
	{
		return versionFileToJson(vfile, saveOrder);
	}
	return QJsonDocument();
}

std::shared_ptr<MinecraftProfile> OneSixVersionFormat::readProfileFromSingleFile(const QJsonObject &obj)
{
	std::shared_ptr<MinecraftProfile> version(new MinecraftProfile(new NullProfileStrategy()));
	try
	{
		version->clear();
		auto file = fromJson(QJsonDocument(obj), QString(), false);
		file->applyTo(version.get());
		version->appendPatch(file);
	}
	catch(Exception &err)
	{
		return nullptr;
	}
	return version;
}
