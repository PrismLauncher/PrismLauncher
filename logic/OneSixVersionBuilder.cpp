/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "OneSixVersionBuilder.h"

#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QDir>
#include <QDebug>

#include "OneSixVersion.h"
#include "OneSixInstance.h"
#include "OneSixRule.h"
#include "modutils.h"
#include "logger/QsLog.h"

#define CURRENT_MINIMUM_LAUNCHER_VERSION 13

struct VersionFile
{
	int order;
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

	struct Library
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
	};
	bool shouldOverwriteLibs = false;
	QList<Library> overwriteLibs;
	QList<Library> addLibs;
	QList<QString> removeLibs;

	enum ApplyError
	{
		LauncherVersionError,
		OtherError,
		NoApplyError
	};

	static Library fromLibraryJson(const QJsonObject &libObj, const QString &filename,
								   bool &isError)
	{
		isError = true;
		Library out;
		if (!libObj.contains("name"))
		{
			QLOG_ERROR() << filename << "contains a library that doesn't have a 'name' field";
			return out;
		}
		out.name = libObj.value("name").toString();

		auto readString = [libObj, filename](const QString &key, QString &variable)
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
				QLOG_ERROR()
					<< filename
					<< "contains a library with an 'extract' field that's not an object";
				return out;
			}
			QJsonObject extractObj = libObj.value("extract").toObject();
			if (!extractObj.contains("exclude") || !extractObj.value("exclude").isArray())
			{
				QLOG_ERROR() << filename
							 << "contains a library with an invalid 'extract' field";
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
				QLOG_ERROR()
					<< filename
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
	static VersionFile fromJson(const QJsonDocument &doc, const QString &filename,
								const bool requireOrder, bool &isError)
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

		auto readString = [root, filename](const QString &key, QString &variable)
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

		readString("id", out.id);
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
				QLOG_ERROR() << filename
							 << "contains a 'tweakers' field, but it's not an array";
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
				QLOG_ERROR() << filename
							 << "contains a '+tweakers' field, but it's not an array";
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
				QLOG_ERROR() << filename
							 << "contains a '-tweakers' field, but it's not an array";
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
			out.shouldOverwriteLibs = true;
			QJsonValue librariesVal = root.value("libraries");
			if (!librariesVal.isArray())
			{
				QLOG_ERROR() << filename
							 << "contains a 'libraries' field, but its not an array";
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
				Library lib = fromLibraryJson(libObj, filename, error);
				if (error)
				{
					QLOG_ERROR() << "Error while reading a library entry in" << filename;
					return out;
				}
				out.overwriteLibs.append(lib);
			}
		}
		if (root.contains("+libraries"))
		{
			QJsonValue librariesVal = root.value("+libraries");
			if (!librariesVal.isArray())
			{
				QLOG_ERROR() << filename
							 << "contains a '+libraries' field, but its not an array";
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
				Library lib = fromLibraryJson(libObj, filename, error);
				if (error)
				{
					QLOG_ERROR() << "Error while reading a library entry in" << filename;
					return out;
				}
				if (!libObj.contains("insert"))
				{
					QLOG_ERROR() << "Missing 'insert' field in '+libraries' field in"
								 << filename;
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
							QLOG_ERROR() << "One library has an empty insert object in"
										 << filename;
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
				QLOG_ERROR() << filename
							 << "contains a '-libraries' field, but its not an array";
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
					QLOG_ERROR() << filename
								 << "contains a library without a valid 'name' field";
					return out;
				}
				out.removeLibs.append(libObj.value("name").toString());
			}
		}

		isError = false;
		return out;
	}

	static std::shared_ptr<OneSixLibrary> createLibrary(const Library &lib)
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
	int findLibrary(QList<std::shared_ptr<OneSixLibrary>> haystack, const QString &needle)
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
	ApplyError applyTo(OneSixVersion *version)
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
			if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard)
					.indexIn(version->id) == -1)
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
				const int index =
					findLibrary(version->libraries,
								QString(lib.name).replace(startOfVersion, INT_MAX, '*'));
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

		OneSixVersion::VersionFile versionFile;
		versionFile.name = name;
		versionFile.id = fileId;
		versionFile.version = this->version;
		versionFile.mcVersion = mcVersion;
		versionFile.filename = filename;
		versionFile.order = order;
		version->versionFiles.append(versionFile);

		return NoApplyError;
	}
};

OneSixVersionBuilder::OneSixVersionBuilder()
{
}

bool OneSixVersionBuilder::build(OneSixVersion *version, OneSixInstance *instance,
								 QWidget *widgetParent, const bool onlyVanilla)
{
	OneSixVersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = instance;
	builder.m_widgetParent = widgetParent;
	return builder.build(onlyVanilla);
}

bool OneSixVersionBuilder::read(OneSixVersion *version, const QJsonObject &obj)
{
	OneSixVersionBuilder builder;
	builder.m_version = version;
	builder.m_instance = 0;
	builder.m_widgetParent = 0;
	return builder.read(obj);
}

bool OneSixVersionBuilder::build(const bool onlyVanilla)
{
	m_version->clear();

	QDir root(m_instance->instanceRoot());
	QDir patches(root.absoluteFilePath("patches/"));

	if (QFile::exists(root.absoluteFilePath("custom.json")))
	{
		QLOG_INFO() << "Reading custom.json";
		VersionFile file;
		if (!read(QFileInfo(root.absoluteFilePath("custom.json")), false, &file))
		{
			return false;
		}
		file.name = "custom.json";
		file.filename = "custom.json";
		file.fileId = "org.multimc.custom.json";
		file.version = QString();
		VersionFile::ApplyError error = file.applyTo(m_version);
		if (error == VersionFile::OtherError)
		{
			QMessageBox::critical(
				m_widgetParent, QObject::tr("Error"),
				QObject::tr(
					"Error while applying %1. Please check MultiMC-0.log for more info.")
					.arg(root.absoluteFilePath("custom.json")));
			return false;
		}
		else if (error == VersionFile::LauncherVersionError)
		{
			QMessageBox::critical(
						m_widgetParent, QObject::tr("Error"),
						QObject::tr("The version descriptors of this instance are not compatible with the current version of MultiMC"));
			return false;
		}
	}
	else
	{
		// version.json -> patches/*.json -> user.json

		// version.json
		{
			QLOG_INFO() << "Reading version.json";
			VersionFile file;
			if (!read(QFileInfo(root.absoluteFilePath("version.json")), false, &file))
			{
				return false;
			}
			file.name = "version.json";
			file.fileId = "org.multimc.version.json";
			file.version = m_instance->intendedVersionId();
			file.mcVersion = m_instance->intendedVersionId();
			VersionFile::ApplyError error = file.applyTo(m_version);
			if (error == VersionFile::OtherError)
			{
				QMessageBox::critical(
					m_widgetParent, QObject::tr("Error"),
					QObject::tr(
						"Error while applying %1. Please check MultiMC-0.log for more info.")
						.arg(root.absoluteFilePath("version.json")));
				return false;
			}
			else if (error == VersionFile::LauncherVersionError)
			{
				QMessageBox::critical(
							m_widgetParent, QObject::tr("Error"),
							QObject::tr("The version descriptors of this instance are not compatible with the current version of MultiMC"));
				return false;
			}
		}

		if (!onlyVanilla)
		{

			// patches/
			{
				// load all, put into map for ordering, apply in the right order
				QMap<QString, int> overrideOrder = readOverrideOrders(m_instance);

				QMap<int, QPair<QString, VersionFile>> files;
				for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
				{
					QLOG_INFO() << "Reading" << info.fileName();
					VersionFile file;
					if (!read(info, true, &file))
					{
						return false;
					}
					if (overrideOrder.contains(file.fileId))
					{
						file.order = overrideOrder.value(file.fileId);
					}
					if (files.contains(file.order))
					{
						QLOG_ERROR() << file.fileId << "has the same order as"
									 << files[file.order].second.fileId;
						return false;
					}
					files.insert(file.order, qMakePair(info.fileName(), file));
				}
				for (auto order : files.keys())
				{
					QLOG_DEBUG() << "Applying file with order" << order;
					auto filePair = files[order];
					VersionFile::ApplyError error = filePair.second.applyTo(m_version);
					if (error == VersionFile::OtherError)
					{
						QMessageBox::critical(
							m_widgetParent, QObject::tr("Error"),
							QObject::tr("Error while applying %1. Please check MultiMC-0.log "
										"for more info.").arg(filePair.first));
						return false;
					}
					else if (error == VersionFile::LauncherVersionError)
					{
						QMessageBox::critical(
									m_widgetParent, QObject::tr("Error"),
									QObject::tr("The version descriptors of this instance are not compatible with the current version of MultiMC"));
						return false;
					}
				}
			}

#if 0
			// user.json
			{
				if (QFile::exists(root.absoluteFilePath("user.json")))
				{
					QLOG_INFO() << "Reading user.json";
					VersionFile file;
					if (!read(QFileInfo(root.absoluteFilePath("user.json")), false, &file))
					{
						return false;
					}
					file.name = "user.json";
					file.fileId = "org.multimc.user.json";
					file.version = QString();
					file.mcVersion = QString();
					bool isError = false;
					file.applyTo(m_version, isError);
					if (isError)
					{
						QMessageBox::critical(
									m_widgetParent, QObject::tr("Error"),
									QObject::tr(
										"Error while applying %1. Please check MultiMC-0.log for more info.")
									.arg(root.absoluteFilePath("user.json")));
						return false;
					}
				}
			}
#endif
		}
	}

	// some final touches
	{
		if (m_version->assets.isEmpty())
		{
			m_version->assets = "legacy";
		}
		if (m_version->minecraftArguments.isEmpty())
		{
			QString toCompare = m_version->processArguments.toLower();
			if (toCompare == "legacy")
			{
				m_version->minecraftArguments = " ${auth_player_name} ${auth_session}";
			}
			else if (toCompare == "username_session")
			{
				m_version->minecraftArguments =
					"--username ${auth_player_name} --session ${auth_session}";
			}
			else if (toCompare == "username_session_version")
			{
				m_version->minecraftArguments = "--username ${auth_player_name} "
												"--session ${auth_session} "
												"--version ${profile_name}";
			}
		}
	}

	return true;
}

bool OneSixVersionBuilder::read(const QJsonObject &obj)
{
	m_version->clear();

	bool isError = false;
	VersionFile file = VersionFile::fromJson(QJsonDocument(obj), QString(), false, isError);
	if (isError)
	{
		QMessageBox::critical(
			m_widgetParent, QObject::tr("Error"),
			QObject::tr("Error while reading. Please check MultiMC-0.log for more info."));
		return false;
	}
	VersionFile::ApplyError error = file.applyTo(m_version);
	if (error == VersionFile::OtherError)
	{
		QMessageBox::critical(
			m_widgetParent, QObject::tr("Error"),
			QObject::tr("Error while applying. Please check MultiMC-0.log for more info."));
		return false;
	}
	else if (error == VersionFile::LauncherVersionError)
	{
		QMessageBox::critical(
					m_widgetParent, QObject::tr("Error"),
					QObject::tr("The version descriptors of this instance are not compatible with the current version of MultiMC"));
		return false;
	}

	return true;
}

bool OneSixVersionBuilder::read(const QFileInfo &fileInfo, const bool requireOrder,
								VersionFile *out)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(
			m_widgetParent, QObject::tr("Error"),
			QObject::tr("Unable to open %1: %2").arg(file.fileName(), file.errorString()));
		return false;
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		QMessageBox::critical(m_widgetParent, QObject::tr("Error"),
							  QObject::tr("Unable to parse %1: %2 at %3")
								  .arg(file.fileName(), error.errorString())
								  .arg(error.offset));
		return false;
	}
	bool isError = false;
	*out = VersionFile::fromJson(doc, file.fileName(), requireOrder, isError);
	if (isError)
	{
		QMessageBox::critical(
			m_widgetParent, QObject::tr("Error"),
			QObject::tr("Error while reading %1. Please check MultiMC-0.log for more info.")
				.arg(file.fileName()));
		;
	}
	return true;
}

QMap<QString, int> OneSixVersionBuilder::readOverrideOrders(OneSixInstance *instance)
{
	QMap<QString, int> out;
	if (QDir(instance->instanceRoot()).exists("order.json"))
	{
		QFile orderFile(instance->instanceRoot() + "/order.json");
		if (!orderFile.open(QFile::ReadOnly))
		{
			QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
						 << " for reading:" << orderFile.errorString();
			QLOG_WARN() << "Ignoring overriden order";
		}
		else
		{
			QJsonParseError error;
			QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
			if (error.error != QJsonParseError::NoError || !doc.isObject())
			{
				QLOG_ERROR() << "Couldn't parse" << orderFile.fileName() << ":"
							 << error.errorString();
				QLOG_WARN() << "Ignoring overriden order";
			}
			else
			{
				QJsonObject obj = doc.object();
				for (auto it = obj.begin(); it != obj.end(); ++it)
				{
					if (it.key().startsWith("org.multimc."))
					{
						continue;
					}
					out.insert(it.key(), it.value().toDouble());
				}
			}
		}
	}
	return out;
}
bool OneSixVersionBuilder::writeOverrideOrders(const QMap<QString, int> &order,
											   OneSixInstance *instance)
{
	QJsonObject obj;
	for (auto it = order.cbegin(); it != order.cend(); ++it)
	{
		if (it.key().startsWith("org.multimc."))
		{
			continue;
		}
		obj.insert(it.key(), it.value());
	}
	QFile orderFile(instance->instanceRoot() + "/order.json");
	if (!orderFile.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
					 << "for writing:" << orderFile.errorString();
		return false;
	}
	orderFile.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	return true;
}
