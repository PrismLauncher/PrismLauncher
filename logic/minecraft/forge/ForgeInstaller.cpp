/* Copyright 2013-2015 MultiMC Contributors
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

#include "ForgeInstaller.h"
#include "ForgeVersionList.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/GradleSpecifier.h"
#include "net/HttpMetaCache.h"
#include "tasks/Task.h"
#include "minecraft/onesix/OneSixInstance.h"
#include <minecraft/onesix/OneSixVersionFormat.h>
#include "minecraft/VersionFilterData.h"
#include "minecraft/MinecraftVersion.h"
#include "Env.h"
#include "Exception.h"
#include <FileSystem.h>

#include <quazip.h>
#include <quazipfile.h>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QCryptographicHash>

ForgeInstaller::ForgeInstaller() : BaseInstaller()
{
}

void ForgeInstaller::prepare(const QString &filename, const QString &universalUrl)
{
	VersionFilePtr newVersion;
	m_universal_url = universalUrl;

	QuaZip zip(filename);
	if (!zip.open(QuaZip::mdUnzip))
		return;

	QuaZipFile file(&zip);

	// read the install profile
	if (!zip.setCurrentFile("install_profile.json"))
		return;

	QJsonParseError jsonError;
	if (!file.open(QIODevice::ReadOnly))
		return;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &jsonError);
	file.close();
	if (jsonError.error != QJsonParseError::NoError)
		return;

	if (!jsonDoc.isObject())
		return;

	QJsonObject root = jsonDoc.object();

	auto installVal = root.value("install");
	auto versionInfoVal = root.value("versionInfo");
	if (!installVal.isObject() || !versionInfoVal.isObject())
		return;

	try
	{
		newVersion = OneSixVersionFormat::versionFileFromJson(QJsonDocument(versionInfoVal.toObject()), QString(), false);
	}
	catch(Exception &err)
	{
		qWarning() << "Forge: Fatal error while parsing version file:" << err.what();
		return;
	}

	for(auto problem: newVersion->getProblems())
	{
		qWarning() << "Forge: Problem found: " << problem.getDescription();
	}
	if(newVersion->getProblemSeverity() == ProblemSeverity::PROBLEM_ERROR)
	{
		qWarning() << "Forge: Errors found while parsing version file";
		return;
	}

	QJsonObject installObj = installVal.toObject();
	QString libraryName = installObj.value("path").toString();
	internalPath = installObj.value("filePath").toString();
	m_forgeVersionString = installObj.value("version").toString().remove("Forge").trimmed();

	// where do we put the library? decode the mojang path
	GradleSpecifier lib(libraryName);

	auto cacheentry = ENV.metacache()->resolveEntry("libraries", lib.toPath());
	finalPath = "libraries/" + lib.toPath();
	if (!FS::ensureFilePathExists(finalPath))
		return;

	if (!zip.setCurrentFile(internalPath))
		return;
	if (!file.open(QIODevice::ReadOnly))
		return;
	{
		QByteArray data = file.readAll();
		// extract file
		QSaveFile extraction(finalPath);
		if (!extraction.open(QIODevice::WriteOnly))
			return;
		if (extraction.write(data) != data.size())
			return;
		if (!extraction.commit())
			return;
		QCryptographicHash md5sum(QCryptographicHash::Md5);
		md5sum.addData(data);

		cacheentry->setStale(false);
		cacheentry->setMD5Sum(md5sum.result().toHex().constData());
		ENV.metacache()->updateEntry(cacheentry);
	}
	file.close();

	m_forge_json = newVersion;
}

bool ForgeInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	if (!m_forge_json)
	{
		return false;
	}

	// A blacklist
	QSet<QString> blacklist{"authlib", "realms"};
	QList<QString> xzlist{"org.scala-lang", "com.typesafe"};

	// get the minecraft version from the instance
	VersionFilePtr minecraft;
	auto minecraftPatch = to->getMinecraftProfile()->versionPatch("net.minecraft");
	if(minecraftPatch)
	{
		minecraft = std::dynamic_pointer_cast<VersionFile>(minecraftPatch);
		if(!minecraft)
		{
			auto mcWrap = std::dynamic_pointer_cast<MinecraftVersion>(minecraftPatch);
			if(mcWrap)
			{
				minecraft = mcWrap->getVersionFile();
			}
		}
	}

	// for each library in the version we are adding (except for the blacklisted)
	QMutableListIterator<LibraryPtr> iter(m_forge_json->libraries);
	while (iter.hasNext())
	{
		auto library = iter.next();
		QString libName = library->artifactId();
		QString libVersion = library->version();
		QString rawName = library->rawName();

		// ignore lwjgl libraries.
		if (g_VersionFilterData.lwjglWhitelist.contains(library->artifactPrefix()))
		{
			iter.remove();
			continue;
		}
		// ignore other blacklisted (realms, authlib)
		if (blacklist.contains(libName))
		{
			iter.remove();
			continue;
		}
		// if minecraft version was found, ignore everything that is already in the minecraft version
		if(minecraft)
		{
			bool found = false;
			for (auto & lib: minecraft->libraries)
			{
				if(library->artifactPrefix() == lib->artifactPrefix() && library->version() == lib->version())
				{
					found = true;
					break;
				}
			}
			if (found)
				continue;
		}

		// if this is the actual forge lib, set an absolute url for the download
		if (m_forge_version->type == ForgeVersion::Gradle)
		{
			if (libName == "forge")
			{
				library->setClassifier("universal");
			}
			else if (libName == "minecraftforge")
			{
				QString forgeCoord("net.minecraftforge:forge:%1:universal");
				// using insane form of the MC version...
				QString longVersion = m_forge_version->mcver + "-" + m_forge_version->jobbuildver;
				GradleSpecifier spec(forgeCoord.arg(longVersion));
				library->setRawName(spec);
			}
		}
		else
		{
			if (libName.contains("minecraftforge"))
			{
				library->setAbsoluteUrl(m_universal_url);
			}
		}

		// mark bad libraries based on the xzlist above
		for (auto entry : xzlist)
		{
			qDebug() << "Testing " << rawName << " : " << entry;
			if (rawName.startsWith(entry))
			{
				library->setHint("forge-pack-xz");
				break;
			}
		}
	}
	QString &args = m_forge_json->minecraftArguments;
	QStringList tweakers;
	{
		QRegularExpression expression("--tweakClass ([a-zA-Z0-9\\.]*)");
		QRegularExpressionMatch match = expression.match(args);
		while (match.hasMatch())
		{
			tweakers.append(match.captured(1));
			args.remove(match.capturedStart(), match.capturedLength());
			match = expression.match(args);
		}
		if(tweakers.size())
		{
			args.operator=(args.trimmed());
			m_forge_json->addTweakers = tweakers;
		}
	}
	if(minecraft && args == minecraft->minecraftArguments)
	{
		args.clear();
	}

	m_forge_json->name = "Forge";
	m_forge_json->fileId = id();
	m_forge_json->version = m_forgeVersionString;
	m_forge_json->dependsOnMinecraftVersion = to->intendedVersionId();
	m_forge_json->minecraftVersion.clear();
	m_forge_json->order = 5;

	QSaveFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		qCritical() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(OneSixVersionFormat::versionFileToJson(m_forge_json, true).toJson());
	file.commit();

	return true;
}

bool ForgeInstaller::addLegacy(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}
	auto entry = ENV.metacache()->resolveEntry("minecraftforge", m_forge_version->filename());
	finalPath = FS::PathCombine(to->jarModsDir(), m_forge_version->filename());
	if (!FS::ensureFilePathExists(finalPath))
	{
		return false;
	}
	if (!QFile::copy(entry->getFullPath(), finalPath))
	{
		return false;
	}
	QJsonObject obj;
	obj.insert("order", 5);
	{
		QJsonArray jarmodsPlus;
		{
			QJsonObject libObj;
			libObj.insert("name", m_forge_version->universal_filename);
			jarmodsPlus.append(libObj);
		}
		obj.insert("+jarMods", jarmodsPlus);
	}

	obj.insert("name", QString("Forge"));
	obj.insert("fileId", id());
	obj.insert("version", m_forge_version->jobbuildver);
	obj.insert("mcVersion", to->intendedVersionId());
	if (g_VersionFilterData.fmlLibsMapping.contains(m_forge_version->mcver))
	{
		QJsonArray traitsPlus;
		traitsPlus.append(QString("legacyFML"));
		obj.insert("+traits", traitsPlus);
	}
	auto fullversion = to->getMinecraftProfile();
	fullversion->remove("net.minecraftforge");

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		qCritical() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();
	return true;
}

class ForgeInstallTask : public Task
{
	Q_OBJECT
public:
	ForgeInstallTask(ForgeInstaller *installer, OneSixInstance *instance,
					 BaseVersionPtr version, QObject *parent = 0)
		: Task(parent), m_installer(installer), m_instance(instance), m_version(version)
	{
	}

protected:
	void executeTask() override
	{
		setStatus(tr("Installing Forge..."));
		ForgeVersionPtr forgeVersion = std::dynamic_pointer_cast<ForgeVersion>(m_version);
		if (!forgeVersion)
		{
			emitFailed(tr("Unknown error occured"));
			return;
		}
		prepare(forgeVersion);
	}
	void prepare(ForgeVersionPtr forgeVersion)
	{
		auto entry = ENV.metacache()->resolveEntry("minecraftforge", forgeVersion->filename());
		auto installFunction = [this, entry, forgeVersion]()
		{
			if (!install(entry, forgeVersion))
			{
				qCritical() << "Failure installing Forge";
				emitFailed(tr("Failure to install Forge"));
			}
			else
			{
				reload();
			}
		};

		/*
		 * HACK IF the local non-stale file is too small, mark is as stale
		 *
		 * This fixes some problems with bad files acquired because of unhandled HTTP redirects
		 * in old versions of MultiMC.
		 */
		if (!entry->isStale())
		{
			QFileInfo localFile(entry->getFullPath());
			if (localFile.size() <= 0x4000)
			{
				entry->setStale(true);
			}
		}

		if (entry->isStale())
		{
			NetJob *fjob = new NetJob("Forge download");
			fjob->addNetAction(CacheDownload::make(forgeVersion->url(), entry));
			connect(fjob, &NetJob::progress, this, &Task::setProgress);
			connect(fjob, &NetJob::status, this, &Task::setStatus);
			connect(fjob, &NetJob::failed, [this](QString reason)
			{ emitFailed(tr("Failure to download Forge:\n%1").arg(reason)); });
			connect(fjob, &NetJob::succeeded, installFunction);
			fjob->start();
		}
		else
		{
			installFunction();
		}
	}
	bool install(const std::shared_ptr<MetaEntry> &entry, const ForgeVersionPtr &forgeVersion)
	{
		if (forgeVersion->usesInstaller())
		{
			QString forgePath = entry->getFullPath();
			m_installer->prepare(forgePath, forgeVersion->universal_url);
			return m_installer->add(m_instance);
		}
		else
			return m_installer->addLegacy(m_instance);
	}
	void reload()
	{
		try
		{
			m_instance->reloadProfile();
			emitSucceeded();
		}
		catch (Exception &e)
		{
			emitFailed(e.cause());
		}
		catch (...)
		{
			emitFailed(tr("Failed to load the version description file for reasons unknown."));
		}
	}

private:
	ForgeInstaller *m_installer;
	OneSixInstance *m_instance;
	BaseVersionPtr m_version;
};

Task *ForgeInstaller::createInstallTask(OneSixInstance *instance,
													BaseVersionPtr version, QObject *parent)
{
	if (!version)
	{
		return nullptr;
	}
	m_forge_version = std::dynamic_pointer_cast<ForgeVersion>(version);
	return new ForgeInstallTask(this, instance, version, parent);
}

#include "ForgeInstaller.moc"
