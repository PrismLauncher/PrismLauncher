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

#include "LiteLoaderVersionList.h"
#include <minecraft/onesix/OneSixVersionFormat.h>
#include "Env.h"
#include "net/URLConstants.h"
#include "Exception.h"

#include <QtXml>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

#include <QtAlgorithms>

#include <QtNetwork>

LiteLoaderVersionList::LiteLoaderVersionList(QObject *parent) : BaseVersionList(parent)
{
}

Task *LiteLoaderVersionList::getLoadTask()
{
	return new LLListLoadTask(this);
}

bool LiteLoaderVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr LiteLoaderVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int LiteLoaderVersionList::count() const
{
	return m_vlist.count();
}

static bool cmpVersions(BaseVersionPtr first, BaseVersionPtr second)
{
	auto left = std::dynamic_pointer_cast<LiteLoaderVersion>(first);
	auto right = std::dynamic_pointer_cast<LiteLoaderVersion>(second);
	return left->timestamp > right->timestamp;
}

VersionFilePtr LiteLoaderVersion::getVersionFile()
{
	auto f = std::make_shared<VersionFile>();
	f->mainClass = "net.minecraft.launchwrapper.Launch";
	f->addTweakers += tweakClass;
	f->order = 10;
	f->libraries = libraries;
	f->name = "LiteLoader";
	f->fileId = "com.mumfrey.liteloader";
	f->version = version;
	f->minecraftVersion = mcVersion;
	return f;
}

void LiteLoaderVersionList::sortVersions()
{
	beginResetModel();
	std::sort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}

QVariant LiteLoaderVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<LiteLoaderVersion>(m_vlist[index.row()]);
	switch (role)
	{
	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	case VersionRole:
		return version->name();

	case VersionIdRole:
		return version->descriptor();

	case ParentGameVersionRole:
		return version->mcVersion;

	case LatestRole:
		return version->isLatest;

	case RecommendedRole:
		return version->isRecommended;

	case TypeRole:
		return version->isSnapshot ? tr("Snapshot") : tr("Release");

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList LiteLoaderVersionList::providesRoles() const
{
	return {VersionPointerRole, VersionRole, VersionIdRole, ParentGameVersionRole, RecommendedRole, LatestRole, TypeRole};
}

BaseVersionPtr LiteLoaderVersionList::getLatestStable() const
{
	for (int i = 0; i < m_vlist.length(); i++)
	{
		auto ver = std::dynamic_pointer_cast<LiteLoaderVersion>(m_vlist.at(i));
		if (ver->isRecommended)
		{
			return m_vlist.at(i);
		}
	}
	return BaseVersionPtr();
}

void LiteLoaderVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	std::sort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}

LLListLoadTask::LLListLoadTask(LiteLoaderVersionList *vlist)
{
	m_list = vlist;
}

LLListLoadTask::~LLListLoadTask()
{
}

void LLListLoadTask::executeTask()
{
	setStatus(tr("Loading LiteLoader version list..."));
	auto job = new NetJob("Version index");
	// we do not care if the version is stale or not.
	auto liteloaderEntry = ENV.metacache()->resolveEntry("liteloader", "versions.json");

	// verify by poking the server.
	liteloaderEntry->setStale(true);

	job->addNetAction(listDownload = Net::Download::makeCached(QUrl(URLConstants::LITELOADER_URL), liteloaderEntry));

	connect(listDownload.get(), SIGNAL(failed(int)), SLOT(listFailed()));

	listJob.reset(job);
	connect(listJob.get(), SIGNAL(succeeded()), SLOT(listDownloaded()));
	connect(listJob.get(), SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	listJob->start();
}

void LLListLoadTask::listFailed()
{
	emitFailed("Failed to load LiteLoader version list.");
	return;
}

void LLListLoadTask::listDownloaded()
{
	QByteArray data;
	{
		auto filename = listDownload->getTargetFilepath();
		QFile listFile(filename);
		if (!listFile.open(QIODevice::ReadOnly))
		{
			emitFailed("Failed to open the LiteLoader version list.");
			return;
		}
		data = listFile.readAll();
		listFile.close();
		listDownload.reset();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed("Error parsing version list JSON:" + jsonError.errorString());
		return;
	}

	if (!jsonDoc.isObject())
	{
		emitFailed("Error parsing version list JSON: jsonDoc is not an object");
		return;
	}

	const QJsonObject root = jsonDoc.object();

	// Now, get the array of versions.
	if (!root.value("versions").isObject())
	{
		emitFailed("Error parsing version list JSON: missing 'versions' object");
		return;
	}

	auto meta = root.value("meta").toObject();
	QString description = meta.value("description").toString(tr("This is a lightweight loader for mods that don't change game mechanics."));
	QString defaultUrl = meta.value("url").toString("http://dl.liteloader.com");
	QString authors = meta.value("authors").toString("Mumfrey");
	auto versions = root.value("versions").toObject();

	QList<BaseVersionPtr> tempList;
	for (auto vIt = versions.begin(); vIt != versions.end(); ++vIt)
	{
		const QString mcVersion = vIt.key();
		const QJsonObject versionObject = vIt.value().toObject();

		auto processArtefacts = [&](QJsonObject artefacts, bool notSnapshots, std::shared_ptr<LiteLoaderVersion> &latest)
		{
			QString latestVersion;
			QList<BaseVersionPtr> perMcVersionList;
			for (auto aIt = artefacts.begin(); aIt != artefacts.end(); ++aIt)
			{
				const QString identifier = aIt.key();
				const QJsonObject artefact = aIt.value().toObject();
				if (identifier == "latest")
				{
					latestVersion = artefact.value("version").toString();
					continue;
				}
				LiteLoaderVersionPtr version(new LiteLoaderVersion());
				version->version = artefact.value("version").toString();
				version->mcVersion = mcVersion;
				version->md5 = artefact.value("md5").toString();
				version->timestamp = artefact.value("timestamp").toString().toLong();
				version->tweakClass = artefact.value("tweakClass").toString();
				version->authors = authors;
				version->description = description;
				version->defaultUrl = defaultUrl;
				version->isSnapshot = !notSnapshots;
				const QJsonArray libs = artefact.value("libraries").toArray();
				for (auto lIt = libs.begin(); lIt != libs.end(); ++lIt)
				{
					auto libobject = (*lIt).toObject();
					try
					{
						auto lib = OneSixVersionFormat::libraryFromJson(libobject, "versions.json");
						// hack to make liteloader 1.7.10_00 work
						if(lib->rawName() == GradleSpecifier("org.ow2.asm:asm-all:5.0.3"))
						{
							lib->setRepositoryURL("http://repo.maven.apache.org/maven2/");
						}
						version->libraries.append(lib);
					}
					catch (Exception &e)
					{
						qCritical() << "Couldn't read JSON object:";
						continue; // FIXME: ignores bad libraries and continues loading
					}
				}
				auto liteloaderLib = std::make_shared<Library>("com.mumfrey:liteloader:" + version->version);
				liteloaderLib->setRepositoryURL("http://dl.liteloader.com/versions/");
				if(!notSnapshots)
				{
					liteloaderLib->setHint("always-stale");
				}
				version->libraries.append(liteloaderLib);
				perMcVersionList.append(version);
			}
			if(notSnapshots)
			{
				for (auto version : perMcVersionList)
				{
					auto v = std::dynamic_pointer_cast<LiteLoaderVersion>(version);
					if(v->version == latestVersion)
					{
						latest = v;
					}
				}
			}
			tempList.append(perMcVersionList);
		};

		std::shared_ptr<LiteLoaderVersion> latestSnapshot;
		std::shared_ptr<LiteLoaderVersion> latestRelease;
		// are there actually released versions for this mc version?
		if(versionObject.contains("artefacts"))
		{
			const QJsonObject artefacts = versionObject.value("artefacts").toObject().value("com.mumfrey:liteloader").toObject();
			processArtefacts(artefacts, true, latestRelease);
		}
		if(versionObject.contains("snapshots"))
		{
			QJsonObject artefacts = versionObject.value("snapshots").toObject().value("com.mumfrey:liteloader").toObject();
			processArtefacts(artefacts, false, latestSnapshot);
		}
		if(latestSnapshot)
		{
			latestSnapshot->isLatest = true;
		}
		else if(latestRelease)
		{
			latestRelease->isLatest = true;
		}
		if(latestRelease)
		{
			latestRelease->isRecommended = true;
		}
	}
	m_list->updateListData(tempList);

	emitSucceeded();
}
