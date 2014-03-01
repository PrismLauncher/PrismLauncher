#include <QJsonArray>
#include <QJsonDocument>

#include <modutils.h>

#include "logger/QsLog.h"
#include "logic/VersionFile.h"
#include "logic/OneSixLibrary.h"
#include "logic/VersionFinal.h"


#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

VersionFile::Library VersionFile::Library::fromJson(const QJsonObject &libObj,
													const QString &filename, bool &isError)
{
	isError = true;
	Library out;
	if (!libObj.contains("name"))
	{
		QLOG_ERROR() << filename << "contains a library that doesn't have a 'name' field";
		return out;
	}
	out.name = libObj.value("name").toString();

	auto readString = [libObj, filename](const QString & key, QString & variable)
	{
		if (libObj.contains(key))
		{
			QJsonValue val = libObj.value(key);
			if (!val.isString())
			{
				QLOG_WARN() << key << "is not a string in" << filename << "(skipping)";
			}
			else
			{
				variable = val.toString();
			}
		}
	};

	readString("url", out.url);
	readString("MMC-hint", out.hint);
	readString("MMC-absulute_url", out.absoluteUrl);
	readString("MMC-absoluteUrl", out.absoluteUrl);
	if (libObj.contains("extract"))
	{
		if (!libObj.value("extract").isObject())
		{
			QLOG_ERROR() << filename
						 << "contains a library with an 'extract' field that's not an object";
			return out;
		}
		QJsonObject extractObj = libObj.value("extract").toObject();
		if (!extractObj.contains("exclude") || !extractObj.value("exclude").isArray())
		{
			QLOG_ERROR() << filename << "contains a library with an invalid 'extract' field";
			return out;
		}
		out.applyExcludes = true;
		QJsonArray excludeArray = extractObj.value("exclude").toArray();
		for (auto excludeVal : excludeArray)
		{
			if (!excludeVal.isString())
			{
				QLOG_WARN() << filename << "contains a library that contains an 'extract' "
										   "field that contains an invalid 'exclude' entry "
										   "(skipping)";
			}
			else
			{
				out.excludes.append(excludeVal.toString());
			}
		}
	}
	if (libObj.contains("natives"))
	{
		if (!libObj.value("natives").isObject())
		{
			QLOG_ERROR() << filename
						 << "contains a library with a 'natives' field that's not an object";
			return out;
		}
		out.applyNatives = true;
		QJsonObject nativesObj = libObj.value("natives").toObject();
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				QLOG_WARN() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys_fromString(it.key());
			if (opSys != Os_Other)
			{
				out.natives.append(qMakePair(opSys, it.value().toString()));
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out.applyRules = true;
		out.rules = rulesFromJsonV4(libObj);
	}
	isError = false;
	return out;
}

VersionFile VersionFile::fromJson(const QJsonDocument &doc, const QString &filename,
								  const bool requireOrder, bool &isError, const bool isFTB)
{
	VersionFile out;
	isError = true;
	if (doc.isEmpty() || doc.isNull())
	{
		QLOG_ERROR() << filename << "is empty or null";
		return out;
	}
	if (!doc.isObject())
	{
		QLOG_ERROR() << "The root of" << filename << "is not an object";
		return out;
	}

	QJsonObject root = doc.object();

	if (requireOrder)
	{
		if (root.contains("order"))
		{
			if (root.value("order").isDouble())
			{
				out.order = root.value("order").toDouble();
			}
			else
			{
				QLOG_ERROR() << "'order' field contains an invalid value in" << filename;
				return out;
			}
		}
		else
		{
			QLOG_ERROR() << filename << "doesn't contain an order field";
		}
	}

	out.name = root.value("name").toString();
	out.fileId = root.value("fileId").toString();
	out.version = root.value("version").toString();
	out.mcVersion = root.value("mcVersion").toString();
	out.filename = filename;

	auto readString = [root, filename](const QString & key, QString & variable)
	{
		if (root.contains(key))
		{
			QJsonValue val = root.value(key);
			if (!val.isString())
			{
				QLOG_WARN() << key << "is not a string in" << filename << "(skipping)";
			}
			else
			{
				variable = val.toString();
			}
		}
	};

	// FTB id attribute is completely bogus. We ignore it.
	if (!isFTB)
	{
		readString("id", out.id);
	}

	readString("mainClass", out.mainClass);
	readString("processArguments", out.processArguments);
	readString("minecraftArguments", out.overwriteMinecraftArguments);
	readString("+minecraftArguments", out.addMinecraftArguments);
	readString("-minecraftArguments", out.removeMinecraftArguments);
	readString("type", out.type);
	readString("releaseTime", out.releaseTime);
	readString("time", out.time);
	readString("assets", out.assets);
	if (root.contains("minimumLauncherVersion"))
	{
		QJsonValue val = root.value("minimumLauncherVersion");
		if (!val.isDouble())
		{
			QLOG_WARN() << "minimumLauncherVersion is not an int in" << filename
						<< "(skipping)";
		}
		else
		{
			out.minimumLauncherVersion = val.toDouble();
		}
	}

	if (root.contains("tweakers"))
	{
		QJsonValue tweakersVal = root.value("tweakers");
		if (!tweakersVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a 'tweakers' field, but it's not an array";
			return out;
		}
		out.shouldOverwriteTweakers = true;
		QJsonArray tweakers = root.value("tweakers").toArray();
		for (auto tweakerVal : tweakers)
		{
			if (!tweakerVal.isString())
			{
				QLOG_ERROR() << filename
							 << "contains a 'tweakers' field entry that's not a string";
				return out;
			}
			out.overwriteTweakers.append(tweakerVal.toString());
		}
	}
	if (root.contains("+tweakers"))
	{
		QJsonValue tweakersVal = root.value("+tweakers");
		if (!tweakersVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a '+tweakers' field, but it's not an array";
			return out;
		}
		QJsonArray tweakers = root.value("+tweakers").toArray();
		for (auto tweakerVal : tweakers)
		{
			if (!tweakerVal.isString())
			{
				QLOG_ERROR() << filename
							 << "contains a '+tweakers' field entry that's not a string";
				return out;
			}
			out.addTweakers.append(tweakerVal.toString());
		}
	}
	if (root.contains("-tweakers"))
	{
		QJsonValue tweakersVal = root.value("-tweakers");
		if (!tweakersVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a '-tweakers' field, but it's not an array";
			return out;
		}
		out.shouldOverwriteTweakers = true;
		QJsonArray tweakers = root.value("-tweakers").toArray();
		for (auto tweakerVal : tweakers)
		{
			if (!tweakerVal.isString())
			{
				QLOG_ERROR() << filename
							 << "contains a '-tweakers' field entry that's not a string";
				return out;
			}
			out.removeTweakers.append(tweakerVal.toString());
		}
	}

	if (root.contains("libraries"))
	{
		out.shouldOverwriteLibs = !isFTB;
		QJsonValue librariesVal = root.value("libraries");
		if (!librariesVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a 'libraries' field, but its not an array";
			return out;
		}
		QJsonArray librariesArray = librariesVal.toArray();
		for (auto libVal : librariesArray)
		{
			if (!libVal.isObject())
			{
				QLOG_ERROR() << filename << "contains a library that's not an object";
				return out;
			}
			QJsonObject libObj = libVal.toObject();
			bool error;
			Library lib = Library::fromJson(libObj, filename, error);
			if (error)
			{
				QLOG_ERROR() << "Error while reading a library entry in" << filename;
				return out;
			}
			if (isFTB)
			{
				lib.hint = "local";
				lib.insertType = Library::Prepend;
				out.addLibs.prepend(lib);
			}
			else
			{
				out.overwriteLibs.append(lib);
			}
		}
	}
	if (root.contains("+libraries"))
	{
		QJsonValue librariesVal = root.value("+libraries");
		if (!librariesVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a '+libraries' field, but its not an array";
			return out;
		}
		QJsonArray librariesArray = librariesVal.toArray();
		for (auto libVal : librariesArray)
		{
			if (!libVal.isObject())
			{
				QLOG_ERROR() << filename << "contains a library that's not an object";
				return out;
			}
			QJsonObject libObj = libVal.toObject();
			bool error;
			Library lib = Library::fromJson(libObj, filename, error);
			if (error)
			{
				QLOG_ERROR() << "Error while reading a library entry in" << filename;
				return out;
			}
			if (!libObj.contains("insert"))
			{
				QLOG_ERROR() << "Missing 'insert' field in '+libraries' field in" << filename;
				return out;
			}
			QJsonValue insertVal = libObj.value("insert");
			QString insertString;
			{
				if (insertVal.isString())
				{
					insertString = insertVal.toString();
				}
				else if (insertVal.isObject())
				{
					QJsonObject insertObj = insertVal.toObject();
					if (insertObj.isEmpty())
					{
						QLOG_ERROR() << "One library has an empty insert object in" << filename;
						return out;
					}
					insertString = insertObj.keys().first();
					lib.insertData = insertObj.value(insertString).toString();
				}
			}
			if (insertString == "apply")
			{
				lib.insertType = Library::Apply;
			}
			else if (insertString == "prepend")
			{
				lib.insertType = Library::Prepend;
			}
			else if (insertString == "append")
			{
				lib.insertType = Library::Prepend;
			}
			else if (insertString == "replace")
			{
				lib.insertType = Library::Replace;
			}
			else
			{
				QLOG_ERROR() << "A '+' library in" << filename
							 << "contains an invalid insert type";
				return out;
			}
			if (libObj.contains("MMC-depend") && libObj.value("MMC-depend").isString())
			{
				const QString dependString = libObj.value("MMC-depend").toString();
				if (dependString == "hard")
				{
					lib.dependType = Library::Hard;
				}
				else if (dependString == "soft")
				{
					lib.dependType = Library::Soft;
				}
				else
				{
					QLOG_ERROR() << "A '+' library in" << filename
								 << "contains an invalid depend type";
					return out;
				}
			}
			out.addLibs.append(lib);
		}
	}
	if (root.contains("-libraries"))
	{
		QJsonValue librariesVal = root.value("-libraries");
		if (!librariesVal.isArray())
		{
			QLOG_ERROR() << filename << "contains a '-libraries' field, but its not an array";
			return out;
		}
		QJsonArray librariesArray = librariesVal.toArray();
		for (auto libVal : librariesArray)
		{
			if (!libVal.isObject())
			{
				QLOG_ERROR() << filename << "contains a library that's not an object";
				return out;
			}
			QJsonObject libObj = libVal.toObject();
			if (!libObj.contains("name"))
			{
				QLOG_ERROR() << filename << "contains a library without a name";
				return out;
			}
			if (!libObj.value("name").isString())
			{
				QLOG_ERROR() << filename << "contains a library without a valid 'name' field";
				return out;
			}
			out.removeLibs.append(libObj.value("name").toString());
		}
	}

	isError = false;
	return out;
}

std::shared_ptr<OneSixLibrary> VersionFile::createLibrary(const VersionFile::Library &lib)
{
	std::shared_ptr<OneSixLibrary> out(new OneSixLibrary(lib.name));
	if (!lib.url.isEmpty())
	{
		out->setBaseUrl(lib.url);
	}
	out->setHint(lib.hint);
	if (!lib.absoluteUrl.isEmpty())
	{
		out->setAbsoluteUrl(lib.absoluteUrl);
	}
	out->setAbsoluteUrl(lib.absoluteUrl);
	out->extract_excludes = lib.excludes;
	for (auto native : lib.natives)
	{
		out->addNative(native.first, native.second);
	}
	out->setRules(lib.rules);
	out->finalize();
	return out;
}

int VersionFile::findLibrary(QList<std::shared_ptr<OneSixLibrary>> haystack,
							 const QString &needle)
{
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (QRegExp(needle, Qt::CaseSensitive, QRegExp::WildcardUnix)
				.indexIn(haystack.at(i)->rawName()) != -1)
		{
			return i;
		}
	}
	return -1;
}

VersionFile::ApplyError VersionFile::applyTo(VersionFinal *version)
{
	if (minimumLauncherVersion != -1)
	{
		if (minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			QLOG_ERROR() << filename << "is for a different launcher version ("
						 << minimumLauncherVersion << "), current supported is"
						 << CURRENT_MINIMUM_LAUNCHER_VERSION;
			return LauncherVersionError;
		}
	}

	if (!version->id.isNull() && !mcVersion.isNull())
	{
		if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(version->id) ==
			-1)
		{
			QLOG_ERROR() << filename << "is for a different version of Minecraft";
			return OtherError;
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
	if (!processArguments.isNull())
	{
		version->processArguments = processArguments;
	}
	if (!type.isNull())
	{
		version->type = type;
	}
	if (!releaseTime.isNull())
	{
		version->releaseTime = releaseTime;
	}
	if (!time.isNull())
	{
		version->time = time;
	}
	if (!assets.isNull())
	{
		version->assets = assets;
	}
	if (minimumLauncherVersion >= 0)
	{
		version->minimumLauncherVersion = minimumLauncherVersion;
	}
	if (!overwriteMinecraftArguments.isNull())
	{
		version->minecraftArguments = overwriteMinecraftArguments;
	}
	if (!addMinecraftArguments.isNull())
	{
		version->minecraftArguments += addMinecraftArguments;
	}
	if (!removeMinecraftArguments.isNull())
	{
		version->minecraftArguments.remove(removeMinecraftArguments);
	}
	if (shouldOverwriteTweakers)
	{
		version->tweakers = overwriteTweakers;
	}
	for (auto tweaker : addTweakers)
	{
		version->tweakers += tweaker;
	}
	for (auto tweaker : removeTweakers)
	{
		version->tweakers.removeAll(tweaker);
	}
	if (shouldOverwriteLibs)
	{
		version->libraries.clear();
		for (auto lib : overwriteLibs)
		{
			version->libraries.append(createLibrary(lib));
		}
	}
	for (auto lib : addLibs)
	{
		switch (lib.insertType)
		{
		case Library::Apply:
		{

			int index = findLibrary(version->libraries, lib.name);
			if (index >= 0)
			{
				auto library = version->libraries[index];
				if (!lib.url.isNull())
				{
					library->setBaseUrl(lib.url);
				}
				if (!lib.hint.isNull())
				{
					library->setHint(lib.hint);
				}
				if (!lib.absoluteUrl.isNull())
				{
					library->setAbsoluteUrl(lib.absoluteUrl);
				}
				if (lib.applyExcludes)
				{
					library->extract_excludes = lib.excludes;
				}
				if (lib.applyNatives)
				{
					library->clearSuffixes();
					for (auto native : lib.natives)
					{
						library->addNative(native.first, native.second);
					}
				}
				if (lib.applyRules)
				{
					library->setRules(lib.rules);
				}
				library->finalize();
			}
			else
			{
				QLOG_WARN() << "Couldn't find" << lib.insertData << "(skipping)";
			}
			break;
		}
		case Library::Append:
		case Library::Prepend:
		{

			const int startOfVersion = lib.name.lastIndexOf(':') + 1;
			const int index = findLibrary(
				version->libraries, QString(lib.name).replace(startOfVersion, INT_MAX, '*'));
			if (index < 0)
			{
				if (lib.insertType == Library::Append)
				{
					version->libraries.append(createLibrary(lib));
				}
				else
				{
					version->libraries.prepend(createLibrary(lib));
				}
			}
			else
			{
				auto otherLib = version->libraries.at(index);
				const Util::Version ourVersion = lib.name.mid(startOfVersion, INT_MAX);
				const Util::Version otherVersion = otherLib->version();
				// if the existing version is a hard dependency we can either use it or
				// fail, but we can't change it
				if (otherLib->dependType == OneSixLibrary::Hard)
				{
					// we need a higher version, or we're hard to and the versions aren't
					// equal
					if (ourVersion > otherVersion ||
						(lib.dependType == Library::Hard && ourVersion != otherVersion))
					{
						QLOG_ERROR() << "Error resolving library dependencies between"
									 << otherLib->rawName() << "and" << lib.name << "in"
									 << filename;
						return OtherError;
					}
					else
					{
						// the library is already existing, so we don't have to do anything
					}
				}
				else if (otherLib->dependType == OneSixLibrary::Soft)
				{
					// if we are higher it means we should update
					if (ourVersion > otherVersion)
					{
						auto library = createLibrary(lib);
						if (Util::Version(otherLib->minVersion) < ourVersion)
						{
							library->minVersion = ourVersion.toString();
						}
						version->libraries.replace(index, library);
					}
					else
					{
						// our version is smaller than the existing version, but we require
						// it: fail
						if (lib.dependType == Library::Hard)
						{
							QLOG_ERROR() << "Error resolving library dependencies between"
										 << otherLib->rawName() << "and" << lib.name << "in"
										 << filename;
							return OtherError;
						}
					}
				}
			}
			break;
		}
		case Library::Replace:
		{
			int index = findLibrary(version->libraries, lib.insertData);
			if (index >= 0)
			{
				version->libraries.replace(index, createLibrary(lib));
			}
			else
			{
				QLOG_WARN() << "Couldn't find" << lib.insertData << "(skipping)";
			}
			break;
		}
		}
	}
	for (auto lib : removeLibs)
	{
		int index = findLibrary(version->libraries, lib);
		if (index >= 0)
		{
			version->libraries.removeAt(index);
		}
		else
		{
			QLOG_WARN() << "Couldn't find" << lib << "(skipping)";
		}
	}

	VersionFinal::VersionFile versionFile;
	versionFile.name = name;
	versionFile.id = fileId;
	versionFile.version = this->version;
	versionFile.mcVersion = mcVersion;
	versionFile.filename = filename;
	versionFile.order = order;
	version->versionFiles.append(versionFile);

	return NoApplyError;
}
