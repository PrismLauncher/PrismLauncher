#include <QJsonArray>
#include <QJsonDocument>

#include <modutils.h>

#include "logger/QsLog.h"
#include "logic/VersionFile.h"
#include "logic/OneSixLibrary.h"
#include "logic/VersionFinal.h"
#include "MMCJson.h"

using namespace MMCJson;

#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

RawLibraryPtr RawLibrary::fromJson(const QJsonObject &libObj, const QString &filename)
{
	RawLibraryPtr out(new RawLibrary());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a library that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();

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

	readString("url", out->url);
	readString("MMC-hint", out->hint);
	readString("MMC-absulute_url", out->absoluteUrl);
	readString("MMC-absoluteUrl", out->absoluteUrl);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = ensureObject(libObj.value("extract"));
		for (auto excludeVal : ensureArray(extractObj.value("exclude")))
		{
			out->excludes.append(ensureString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		out->applyNatives = true;
		QJsonObject nativesObj = ensureObject(libObj.value("natives"));
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				QLOG_WARN() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys_fromString(it.key());
			if (opSys != Os_Other)
			{
				out->natives.append(qMakePair(opSys, it.value().toString()));
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->rules = rulesFromJsonV4(libObj);
	}
	return out;
}

VersionFilePtr VersionFile::fromJson(const QJsonDocument &doc, const QString &filename,
								  const bool requireOrder, const bool isFTB)
{
	VersionFilePtr out(new VersionFile());
	if (doc.isEmpty() || doc.isNull())
	{
		throw JSONValidationError(filename + " is empty or null");
	}
	if (!doc.isObject())
	{
		throw JSONValidationError("The root of " + filename + " is not an object");
	}

	QJsonObject root = doc.object();

	if (requireOrder)
	{
		if (root.contains("order"))
		{
			out->order = ensureInteger(root.value("order"));
		}
		else
		{
			// FIXME: evaluate if we don't want to throw exceptions here instead
			QLOG_ERROR() << filename << "doesn't contain an order field";
		}
	}

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	out->mcVersion = root.value("mcVersion").toString();
	out->filename = filename;

	auto readString = [root, filename](const QString & key, QString & variable)
	{
		if (root.contains(key))
		{
			variable = ensureString(root.value(key));
		}
	};

	// FIXME: This should be ignored when applying.
	if (!isFTB)
	{
		readString("id", out->id);
	}

	readString("mainClass", out->mainClass);
	readString("processArguments", out->processArguments);
	readString("minecraftArguments", out->overwriteMinecraftArguments);
	readString("+minecraftArguments", out->addMinecraftArguments);
	readString("-minecraftArguments", out->removeMinecraftArguments);
	readString("type", out->type);
	readString("releaseTime", out->releaseTime);
	readString("time", out->time);
	readString("assets", out->assets);

	if (root.contains("minimumLauncherVersion"))
	{
		out->minimumLauncherVersion = ensureInteger(root.value("minimumLauncherVersion"));
	}

	if (root.contains("tweakers"))
	{
		out->shouldOverwriteTweakers = true;
		for (auto tweakerVal : ensureArray(root.value("tweakers")))
		{
			out->overwriteTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("+tweakers"))
	{
		for (auto tweakerVal : ensureArray(root.value("+tweakers")))
		{
			out->addTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("-tweakers"))
	{
		for (auto tweakerVal : ensureArray(root.value("-tweakers")))
		{
			out->removeTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("libraries"))
	{
		// FIXME: This should be done when applying.
		out->shouldOverwriteLibs = !isFTB;
		for (auto libVal : ensureArray(root.value("libraries")))
		{
			auto libObj = ensureObject(libVal);

			auto lib = RawLibrary::fromJson(libObj, filename);
			// FIXME: This should be done when applying.
			if (isFTB)
			{
				lib->hint = "local";
				lib->insertType = RawLibrary::Prepend;
				out->addLibs.prepend(lib);
			}
			else
			{
				out->overwriteLibs.append(lib);
			}
		}
	}

	if (root.contains("+libraries"))
	{
		for (auto libVal : ensureArray(root.value("+libraries")))
		{
			QJsonObject libObj = ensureObject(libVal);
			QJsonValue insertVal = ensureExists(libObj.value("insert"));

			// parse the library
			auto lib = RawLibrary::fromJson(libObj, filename);

			// TODO: utility functions for handling this case. templates?
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
						throw JSONValidationError("One library has an empty insert object in " +
												  filename);
					}
					insertString = insertObj.keys().first();
					lib->insertData = insertObj.value(insertString).toString();
				}
			}
			if (insertString == "apply")
			{
				lib->insertType = RawLibrary::Apply;
			}
			else if (insertString == "prepend")
			{
				lib->insertType = RawLibrary::Prepend;
			}
			else if (insertString == "append")
			{
				lib->insertType = RawLibrary::Prepend;
			}
			else if (insertString == "replace")
			{
				lib->insertType = RawLibrary::Replace;
			}
			else
			{
				throw JSONValidationError("A '+' library in " + filename +
										  " contains an invalid insert type");
			}
			if (libObj.contains("MMC-depend"))
			{
				const QString dependString = ensureString(libObj.value("MMC-depend"));
				if (dependString == "hard")
				{
					lib->dependType = RawLibrary::Hard;
				}
				else if (dependString == "soft")
				{
					lib->dependType = RawLibrary::Soft;
				}
				else
				{
					throw JSONValidationError("A '+' library in " + filename +
											  " contains an invalid depend type");
				}
			}
			out->addLibs.append(lib);
		}
	}
	if (root.contains("-libraries"))
	{
		for (auto libVal : ensureArray(root.value("-libraries")))
		{
			auto libObj = ensureObject(libVal);
			out->removeLibs.append(ensureString(libObj.value("name")));
		}
	}
	return out;
}

OneSixLibraryPtr VersionFile::createLibrary(RawLibraryPtr lib)
{
	std::shared_ptr<OneSixLibrary> out(new OneSixLibrary(lib->name));
	if (!lib->url.isEmpty())
	{
		out->setBaseUrl(lib->url);
	}
	out->setHint(lib->hint);
	if (!lib->absoluteUrl.isEmpty())
	{
		out->setAbsoluteUrl(lib->absoluteUrl);
	}
	out->setAbsoluteUrl(lib->absoluteUrl);
	out->extract_excludes = lib->excludes;
	for (auto native : lib->natives)
	{
		out->addNative(native.first, native.second);
	}
	out->setRules(lib->rules);
	out->finalize();
	return out;
}

int VersionFile::findLibrary(QList<OneSixLibraryPtr> haystack, const QString &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		QString chunk = haystack.at(i)->rawName();
		if (QRegExp(needle, Qt::CaseSensitive, QRegExp::WildcardUnix).indexIn(chunk) != -1)
		{
			// only one is allowed.
			if(retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

void VersionFile::applyTo(VersionFinal *version)
{
	if (minimumLauncherVersion != -1)
	{
		if (minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			throw LauncherVersionError(minimumLauncherVersion, CURRENT_MINIMUM_LAUNCHER_VERSION);
		}
	}

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
		switch (lib->insertType)
		{
		case RawLibrary::Apply:
		{
			// QLOG_INFO() << "Applying lib " << lib->name;
			int index = findLibrary(version->libraries, lib->name);
			if (index >= 0)
			{
				auto library = version->libraries[index];
				if (!lib->url.isNull())
				{
					library->setBaseUrl(lib->url);
				}
				if (!lib->hint.isNull())
				{
					library->setHint(lib->hint);
				}
				if (!lib->absoluteUrl.isNull())
				{
					library->setAbsoluteUrl(lib->absoluteUrl);
				}
				if (lib->applyExcludes)
				{
					library->extract_excludes = lib->excludes;
				}
				if (lib->applyNatives)
				{
					library->clearSuffixes();
					for (auto native : lib->natives)
					{
						library->addNative(native.first, native.second);
					}
				}
				if (lib->applyRules)
				{
					library->setRules(lib->rules);
				}
				library->finalize();
			}
			else
			{
				QLOG_WARN() << "Couldn't find" << lib->name << "(skipping)";
			}
			break;
		}
		case RawLibrary::Append:
		case RawLibrary::Prepend:
		{
			// QLOG_INFO() << "Adding lib " << lib->name;
			const int startOfVersion = lib->name.lastIndexOf(':') + 1;
			const int index = findLibrary(
				version->libraries, QString(lib->name).replace(startOfVersion, INT_MAX, '*'));
			if (index < 0)
			{
				if (lib->insertType == RawLibrary::Append)
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
				const Util::Version ourVersion = lib->name.mid(startOfVersion, INT_MAX);
				const Util::Version otherVersion = otherLib->version();
				// if the existing version is a hard dependency we can either use it or
				// fail, but we can't change it
				if (otherLib->dependType == OneSixLibrary::Hard)
				{
					// we need a higher version, or we're hard to and the versions aren't
					// equal
					if (ourVersion > otherVersion ||
						(lib->dependType == RawLibrary::Hard && ourVersion != otherVersion))
					{
						throw VersionBuildError(
							QObject::tr(
								"Error resolving library dependencies between %1 and %2 in %3.")
								.arg(otherLib->rawName(), lib->name, filename));
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
						if (lib->dependType == RawLibrary::Hard)
						{
							throw VersionBuildError(QObject::tr(
								"Error resolving library dependencies between %1 and %2 in %3.")
														.arg(otherLib->rawName(), lib->name,
															 filename));
						}
					}
				}
			}
			break;
		}
		case RawLibrary::Replace:
		{
			QString toReplace;
			if(lib->insertData.isEmpty())
			{
				const int startOfVersion = lib->name.lastIndexOf(':') + 1;
				toReplace = QString(lib->name).replace(startOfVersion, INT_MAX, '*');
			}
			else
				toReplace = lib->insertData;
			// QLOG_INFO() << "Replacing lib " << toReplace << " with " << lib->name;
			int index = findLibrary(version->libraries, toReplace);
			if (index >= 0)
			{
				version->libraries.replace(index, createLibrary(lib));
			}
			else
			{
				QLOG_WARN() << "Couldn't find" << toReplace << "(skipping)";
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
			// QLOG_INFO() << "Removing lib " << lib;
			version->libraries.removeAt(index);
		}
		else
		{
			QLOG_WARN() << "Couldn't find" << lib << "(skipping)";
		}
	}
}
