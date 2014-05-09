#include <QJsonArray>
#include <QJsonDocument>
#include <modutils.h>

#include "logger/QsLog.h"

#include "logic/minecraft/VersionFile.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/minecraft/VersionFinal.h"
#include "logic/minecraft/JarMod.h"
#include "ParseUtils.h"

#include "logic/MMCJson.h"
using namespace MMCJson;

#include "VersionBuildError.h"

#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

int findLibrary(QList<OneSixLibraryPtr> haystack, const QString &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		QString chunk = haystack.at(i)->rawName();
		if (QRegExp(needle, Qt::CaseSensitive, QRegExp::WildcardUnix).indexIn(chunk) != -1)
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
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

	auto readString = [root](const QString & key, QString & variable)
	{
		if (root.contains(key))
		{
			variable = ensureString(root.value(key));
		}
	};

	auto readStringRet = [root](const QString & key)->QString
	{
		if (root.contains(key))
		{
			return ensureString(root.value(key));
		}
		return QString();
	}
	;

	// FIXME: This should be ignored when applying.
	if (!isFTB)
	{
		readString("id", out->id);
	}

	readString("mainClass", out->mainClass);
	readString("appletClass", out->appletClass);
	readString("processArguments", out->processArguments);
	readString("minecraftArguments", out->overwriteMinecraftArguments);
	readString("+minecraftArguments", out->addMinecraftArguments);
	readString("-minecraftArguments", out->removeMinecraftArguments);
	readString("type", out->type);
	if (out->isVanilla())
	{
		parse_timestamp(readStringRet("releaseTime"), out->m_releaseTimeString,
						out->m_releaseTime);
		parse_timestamp(readStringRet("time"), out->m_updateTimeString, out->m_updateTime);
	}

	readStringRet("time");
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

	if (root.contains("+traits"))
	{
		for (auto tweakerVal : ensureArray(root.value("+traits")))
		{
			out->traits.insert(ensureString(tweakerVal));
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

	if (root.contains("+jarMods"))
	{
		for (auto libVal : ensureArray(root.value("+jarMods")))
		{
			QJsonObject libObj = ensureObject(libVal);
			// parse the jarmod
			auto lib = Jarmod::fromJson(libObj, filename);
			// and add to jar mods
			out->jarMods.append(lib);
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

bool VersionFile::isVanilla()
{
	return fileId == "org.multimc.version.json";
}

bool VersionFile::hasJarMods()
{
	return !jarMods.isEmpty();
}

void VersionFile::applyTo(VersionFinal *version)
{
	if (minimumLauncherVersion != -1)
	{
		if (minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			throw LauncherVersionError(minimumLauncherVersion,
									   CURRENT_MINIMUM_LAUNCHER_VERSION);
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
	if (!appletClass.isNull())
	{
		version->appletClass = appletClass;
	}
	if (!processArguments.isNull())
	{
		if (isVanilla())
		{
			version->vanillaProcessArguments = processArguments;
		}
		version->processArguments = processArguments;
	}
	if(isVanilla())
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
		if(version->minimumLauncherVersion < minimumLauncherVersion)
			version->minimumLauncherVersion = minimumLauncherVersion;
	}
	if (!overwriteMinecraftArguments.isNull())
	{
		if (isVanilla())
		{
			version->vanillaMinecraftArguments = overwriteMinecraftArguments;
		}
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
	version->jarMods.append(jarMods);
	version->traits.unite(traits);
	if (shouldOverwriteLibs)
	{
		QList<OneSixLibraryPtr> libs;
		for (auto lib : overwriteLibs)
		{
			libs.append(createLibrary(lib));
		}
		if (isVanilla())
			version->vanillaLibraries = libs;
		version->libraries = libs;
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
			if (lib->insertData.isEmpty())
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
