/* Copyright 2013-2014 MultiMC Contributors
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
#include "logic/minecraft/InstanceVersion.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/net/HttpMetaCache.h"
#include "logic/tasks/Task.h"
#include "logic/OneSixInstance.h"
#include "logic/forge/ForgeVersionList.h"
#include "logic/VersionFilterData.h"
#include "gui/dialogs/ProgressDialog.h"

#include <quazip.h>
#include <quazipfile.h>
#include <pathutils.h>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "MultiMC.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QCryptographicHash>

ForgeInstaller::ForgeInstaller() : BaseInstaller()
{
}
void ForgeInstaller::prepare(const QString &filename, const QString &universalUrl)
{
	std::shared_ptr<InstanceVersion> newVersion;
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

	// read the forge version info
	{
		newVersion = InstanceVersion::fromJson(versionInfoVal.toObject());
		if (!newVersion)
			return;
	}

	QJsonObject installObj = installVal.toObject();
	QString libraryName = installObj.value("path").toString();
	internalPath = installObj.value("filePath").toString();
	m_forgeVersionString = installObj.value("version").toString().remove("Forge").trimmed();

	// where do we put the library? decode the mojang path
	OneSixLibrary lib(libraryName);

	auto cacheentry = MMC->metacache()->resolveEntry("libraries", lib.storagePath());
	finalPath = "libraries/" + lib.storagePath();
	if (!ensureFilePathExists(finalPath))
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

		cacheentry->stale = false;
		cacheentry->md5sum = md5sum.result().toHex().constData();
		MMC->metacache()->updateEntry(cacheentry);
	}
	file.close();

	m_forge_json = newVersion;
	realVersionId = m_forge_json->id = installObj.value("minecraft").toString();
}
bool ForgeInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;
	obj.insert("order", 5);

	if (!m_forge_json)
		return false;
	int sliding_insert_window = 0;
	{
		QJsonArray librariesPlus;
		// A blacklist
		QSet<QString> blacklist{"authlib", "realms"};
		// 
		QList<QString> xzlist{"org.scala-lang", "com.typesafe"};
		// for each library in the version we are adding (except for the blacklisted)
		for (auto lib : m_forge_json->libraries)
		{
			QString libName = lib->artifactId();
			QString rawName = lib->rawName();

			// ignore lwjgl libraries.
			if (g_VersionFilterData.lwjglWhitelist.contains(lib->artifactPrefix()))
				continue;
			// ignore other blacklisted (realms, authlib)
			if (blacklist.contains(libName))
				continue;

			// WARNING: This could actually break.
			// if this is the actual forge lib, set an absolute url for the download
			if(m_forge_version->type == ForgeVersion::Gradle)
			{
				if (libName == "forge")
				{
					lib->setClassifier("universal");
				}
				else if (libName == "minecraftforge")
				{
					QString forgeCoord ("net.minecraftforge:forge:%1:universal");
					// using insane form of the MC version...
					QString longVersion = m_forge_version->mcver + "-" + m_forge_version->jobbuildver;
					GradleSpecifier spec(forgeCoord.arg(longVersion));
					lib->setRawName(spec);
				}
			}
			else
			{
				if (libName.contains("minecraftforge"))
				{
					lib->setAbsoluteUrl(m_universal_url);
				}
			}

			// WARNING: This could actually break.
			// mark bad libraries based on the xzlist above
			for(auto entry : xzlist)
			{
				QLOG_DEBUG() << "Testing " << rawName << " : " << entry;
				if(rawName.startsWith(entry))
				{
					lib->setHint("forge-pack-xz");
					break;
				}
			}

			QJsonObject libObj = lib->toJson();

			bool found = false;
			bool equals = false;
			// find an entry that matches this one
			for (auto tolib : to->getFullVersion()->vanillaLibraries)
			{
				if (tolib->artifactId() != libName)
					continue;
				found = true;
				if (tolib->toJson() == libObj)
				{
					equals = true;
				}
				// replace lib
				libObj.insert("insert", QString("replace"));
				break;
			}
			if (equals)
			{
				continue;
			}
			if (!found)
			{
				// add lib
				libObj.insert("insert", QString("prepend"));
				if (lib->artifactId() == "minecraftforge" || lib->artifactId() == "forge")
				{
					libObj.insert("MMC-depend", QString("hard"));
				}
				sliding_insert_window++;
			}
			librariesPlus.prepend(libObj);
		}
		obj.insert("+libraries", librariesPlus);
		obj.insert("mainClass", m_forge_json->mainClass);
		QString args = m_forge_json->minecraftArguments;
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
		}
		if (!args.isEmpty() && args != to->getFullVersion()->vanillaMinecraftArguments)
		{
			obj.insert("minecraftArguments", args);
		}
		if (!tweakers.isEmpty())
		{
			obj.insert("+tweakers", QJsonArray::fromStringList(tweakers));
		}
		if (!m_forge_json->processArguments.isEmpty() &&
			m_forge_json->processArguments != to->getFullVersion()->vanillaProcessArguments)
		{
			obj.insert("processArguments", m_forge_json->processArguments);
		}
	}

	obj.insert("name", QString("Forge"));
	obj.insert("fileId", id());
	obj.insert("version", m_forgeVersionString);
	obj.insert("mcVersion", to->intendedVersionId());

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}

bool ForgeInstaller::addLegacy(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}
	auto entry = MMC->metacache()->resolveEntry("minecraftforge", m_forge_version->filename());
	finalPath = PathCombine(to->jarModsDir(), m_forge_version->filename());
	if (!ensureFilePathExists(finalPath))
	{
		return false;
	}
	if (!QFile::copy(entry->getFullPath(),finalPath))
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
	auto fullversion = to->getFullVersion();
	fullversion->remove("net.minecraftforge");
	
	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
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
		setStatus(tr("Installing forge..."));
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
		auto entry = MMC->metacache()->resolveEntry("minecraftforge", forgeVersion->filename());
		auto installFunction = [this, entry, forgeVersion]()
		{
			if (!install(entry, forgeVersion))
			{
				QLOG_ERROR() << "Failure installing forge";
				emitFailed(tr("Failure to install forge"));
			}
			else
			{
				reload();
			}
		};

		if (entry->stale)
		{
			NetJob *fjob = new NetJob("Forge download");
			fjob->addNetAction(CacheDownload::make(forgeVersion->url(), entry));
			connect(fjob, &NetJob::progress, [this](qint64 current, qint64 total)
			{ setProgress(100 * current / qMax((qint64)1, total)); });
			connect(fjob, &NetJob::status, [this](const QString & msg)
			{ setStatus(msg); });
			connect(fjob, &NetJob::failed, [this]()
			{ emitFailed(tr("Failure to download forge")); });
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
			m_instance->reloadVersion();
			emitSucceeded();
		}
		catch (MMCError &e)
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

ProgressProvider *ForgeInstaller::createInstallTask(OneSixInstance *instance,
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
