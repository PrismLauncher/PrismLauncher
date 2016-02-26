#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/RawLibrary.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/JarMod.h"
#include "ParseUtils.h"

#include "Json.h"
using namespace Json;

#include "VersionBuildError.h"
#include <Version.h>

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

int findLibraryByName(QList<RawLibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

void checkMinimumLauncherVersion(VersionFilePtr out)
{
	const int CURRENT_MINIMUM_LAUNCHER_VERSION = 14;
	if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
	{
		out->addProblem(
			PROBLEM_WARNING,
			QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
				.arg(out->minimumLauncherVersion)
				.arg(CURRENT_MINIMUM_LAUNCHER_VERSION));
	}
}

VersionFilePtr VersionFile::fromMojangJson(const QJsonDocument &doc, const QString &filename)
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

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	out->mcVersion = root.value("mcVersion").toString();
	out->filename = filename;

	readString(root, "id", out->id);

	readString(root, "mainClass", out->mainClass);
	readString(root, "appletClass", out->appletClass);
	readString(root, "minecraftArguments", out->overwriteMinecraftArguments);
	readString(root, "type", out->type);

	readString(root, "assets", out->assets);

	if (root.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = requireInteger(root.value("minimumLauncherVersion"));
		checkMinimumLauncherVersion(out);
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
	return out;
}

VersionFilePtr VersionFile::fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder)
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

	if (root.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = requireInteger(root.value("minimumLauncherVersion"));
		checkMinimumLauncherVersion(out);
	}

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

QJsonDocument VersionFile::toJson(bool saveOrder)
{
	QJsonObject root;
	if (saveOrder)
	{
		root.insert("order", order);
	}
	writeString(root, "name", name);
	writeString(root, "fileId", fileId);
	writeString(root, "version", version);
	writeString(root, "mcVersion", mcVersion);
	writeString(root, "id", id);
	writeString(root, "mainClass", mainClass);
	writeString(root, "appletClass", appletClass);
	writeString(root, "processArguments", processArguments);
	writeString(root, "minecraftArguments", overwriteMinecraftArguments);
	writeString(root, "+minecraftArguments", addMinecraftArguments);
	writeString(root, "type", type);
	writeString(root, "assets", assets);
	if (isMinecraftVersion())
	{
		writeString(root, "releaseTime", m_releaseTimeString);
		writeString(root, "time", m_updateTimeString);
	}
	if (minimumLauncherVersion != -1)
	{
		root.insert("minimumLauncherVersion", minimumLauncherVersion);
	}
	writeStringList(root, "tweakers", overwriteTweakers);
	writeStringList(root, "+tweakers", addTweakers);
	writeStringList(root, "+traits", traits.toList());
	writeObjectList(root, "libraries", overwriteLibs);
	writeObjectList(root, "+libraries", addLibs);
	writeObjectList(root, "+jarMods", jarMods);
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

bool VersionFile::isMinecraftVersion()
{
	return fileId == "net.minecraft";
}

bool VersionFile::hasJarMods()
{
	return !jarMods.isEmpty();
}

void VersionFile::applyTo(MinecraftProfile *version)
{
	if (!version->id.isNull() && !mcVersion.isNull())
	{
		if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(version->id) ==
			-1)
		{
			throw MinecraftVersionMismatch(fileId, mcVersion, version->id);
		}
	}

	if (!id.isNull())
	{
		version->id = id;
	}
	if (!mainClass.isNull())
	{
		version->mainClass = mainClass;
	}
	if (!appletClass.isNull())
	{
		version->appletClass = appletClass;
	}
	if (!processArguments.isNull())
	{
		if (isMinecraftVersion())
		{
			version->vanillaProcessArguments = processArguments;
		}
		version->processArguments = processArguments;
	}
	if (isMinecraftVersion())
	{
		if (!type.isNull())
		{
			version->type = type;
		}
		if (!m_releaseTimeString.isNull())
		{
			version->m_releaseTimeString = m_releaseTimeString;
			version->m_releaseTime = m_releaseTime;
		}
		if (!m_updateTimeString.isNull())
		{
			version->m_updateTimeString = m_updateTimeString;
			version->m_updateTime = m_updateTime;
		}
	}
	if (!assets.isNull())
	{
		version->assets = assets;
	}
	if (minimumLauncherVersion >= 0)
	{
		if (version->minimumLauncherVersion < minimumLauncherVersion)
			version->minimumLauncherVersion = minimumLauncherVersion;
	}
	if (!overwriteMinecraftArguments.isNull())
	{
		if (isMinecraftVersion())
		{
			version->vanillaMinecraftArguments = overwriteMinecraftArguments;
		}
		version->minecraftArguments = overwriteMinecraftArguments;
	}
	if (!addMinecraftArguments.isNull())
	{
		version->minecraftArguments += addMinecraftArguments;
	}
	if (shouldOverwriteTweakers)
	{
		version->tweakers = overwriteTweakers;
	}
	for (auto tweaker : addTweakers)
	{
		version->tweakers += tweaker;
	}
	version->jarMods.append(jarMods);
	version->traits.unite(traits);
	if (shouldOverwriteLibs)
	{
		QList<RawLibraryPtr> libs;
		for (auto lib : overwriteLibs)
		{
			libs.append(RawLibrary::limitedCopy(lib));
		}
		if (isMinecraftVersion())
		{
			version->vanillaLibraries = libs;
		}
		version->libraries = libs;
	}
	for (auto addedLibrary : addLibs)
	{
		// find the library by name.
		const int index = findLibraryByName(version->libraries, addedLibrary->rawName());
		// library not found? just add it.
		if (index < 0)
		{
			version->libraries.append(RawLibrary::limitedCopy(addedLibrary));
			continue;
		}
		auto existingLibrary = version->libraries.at(index);
		// if we are higher it means we should update
		if (Version(addedLibrary->version()) > Version(existingLibrary->version()))
		{
			auto library = RawLibrary::limitedCopy(addedLibrary);
			version->libraries.replace(index, library);
		}
	}
}
