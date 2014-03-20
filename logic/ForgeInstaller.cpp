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

#include "ForgeInstaller.h"
#include "VersionFinal.h"
#include "OneSixLibrary.h"
#include "net/HttpMetaCache.h"
#include <quazip.h>
#include <quazipfile.h>
#include <pathutils.h>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "MultiMC.h"
#include "tasks/Task.h"
#include "OneSixInstance.h"
#include "lists/ForgeVersionList.h"
#include "gui/dialogs/ProgressDialog.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QCryptographicHash>

ForgeInstaller::ForgeInstaller()
	: BaseInstaller()
{
}
void ForgeInstaller::prepare(const QString &filename, const QString &universalUrl)
{
	std::shared_ptr<VersionFinal> newVersion;
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
		newVersion = VersionFinal::fromJson(versionInfoVal.toObject());
		if (!newVersion)
			return;
	}

	QJsonObject installObj = installVal.toObject();
	QString libraryName = installObj.value("path").toString();
	internalPath = installObj.value("filePath").toString();
	m_forgeVersionString = installObj.value("version").toString().remove("Forge").trimmed();

	// where do we put the library? decode the mojang path
	OneSixLibrary lib(libraryName);
	lib.finalize();

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

	m_forge_version = newVersion;
	realVersionId = m_forge_version->id = installObj.value("minecraft").toString();
}
bool ForgeInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;
	obj.insert("order", 5);

	if (!m_forge_version)
		return false;
	int sliding_insert_window = 0;
	{
		QJsonArray librariesPlus;

		// for each library in the version we are adding (except for the blacklisted)
		QSet<QString> blacklist{"lwjgl", "lwjgl_util", "lwjgl-platform"};
		for (auto lib : m_forge_version->libraries)
		{
			QString libName = lib->name();
			// WARNING: This could actually break.
			// if this is the actual forge lib, set an absolute url for the download
			if (libName.contains("minecraftforge"))
			{
				lib->setAbsoluteUrl(m_universal_url);
			}
			else if (libName.contains("scala"))
			{
				lib->setHint("forge-pack-xz");
			}
			if (blacklist.contains(libName))
				continue;

			QJsonObject libObj = lib->toJson();

			bool found = false;
			bool equals = false;
			// find an entry that matches this one
			for (auto tolib : to->getVanillaVersion()->libraries)
			{
				if (tolib->name() != libName)
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
				if (lib->name() == "minecraftforge")
				{
					libObj.insert("MMC-depend", QString("hard"));
				}
				sliding_insert_window++;
			}
			librariesPlus.prepend(libObj);
		}
		obj.insert("+libraries", librariesPlus);
		obj.insert("mainClass", m_forge_version->mainClass);
		QString args = m_forge_version->minecraftArguments;
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
		if (!args.isEmpty() && args != to->getVanillaVersion()->minecraftArguments)
		{
			obj.insert("minecraftArguments", args);
		}
		if (!tweakers.isEmpty())
		{
			obj.insert("+tweakers", QJsonArray::fromStringList(tweakers));
		}
		if (!m_forge_version->processArguments.isEmpty() &&
			m_forge_version->processArguments != to->getVanillaVersion()->processArguments)
		{
			obj.insert("processArguments", m_forge_version->processArguments);
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

class ForgeInstallTask : public Task
{
	Q_OBJECT
public:
	ForgeInstallTask(ForgeInstaller *installer, OneSixInstance *instance, BaseVersionPtr version, QObject *parent = 0)
		: Task(parent), m_installer(installer), m_instance(instance), m_version(version)
	{
	}

protected:
	void executeTask() override
	{
		{
			setStatus(tr("Installing forge..."));
			ForgeVersionPtr forgeVersion =
				std::dynamic_pointer_cast<ForgeVersion>(m_version);
			if (!forgeVersion)
			{
				emitFailed(tr("Unknown error occured"));
				return;
			}
			auto entry = MMC->metacache()->resolveEntry("minecraftforge", forgeVersion->filename);
			if (entry->stale)
			{
				NetJob *fjob = new NetJob("Forge download");
				fjob->addNetAction(CacheDownload::make(forgeVersion->installer_url, entry));
				connect(fjob, &NetJob::progress, [this](qint64 current, qint64 total){setProgress(100 * current / qMax((qint64)1, total));});
				connect(fjob, &NetJob::status, [this](const QString &msg){setStatus(msg);});
				connect(fjob, &NetJob::failed, [this](){emitFailed(tr("Failure to download forge"));});
				connect(fjob, &NetJob::succeeded, [this, entry, forgeVersion]()
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
				});
				fjob->start();
			}
			else
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
			}
		}
	}

	bool install(const std::shared_ptr<MetaEntry> &entry, const ForgeVersionPtr &forgeVersion)
	{
		QString forgePath = entry->getFullPath();
		m_installer->prepare(forgePath, forgeVersion->universal_url);
		return m_installer->add(m_instance);
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

ProgressProvider *ForgeInstaller::createInstallTask(OneSixInstance *instance, BaseVersionPtr version, QObject *parent)
{
	return new ForgeInstallTask(this, instance, version, parent);
}

#include "ForgeInstaller.moc"
