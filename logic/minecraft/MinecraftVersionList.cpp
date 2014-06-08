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

#include <QtXml>
#include "logic/MMCJson.h"
#include <QtAlgorithms>
#include <QtNetwork>

#include "MultiMC.h"
#include "MMCError.h"

#include "MinecraftVersionList.h"
#include "logic/net/URLConstants.h"

#include "ParseUtils.h"
#include "VersionBuilder.h"
#include <logic/VersionFilterData.h>
#include <pathutils.h>

static const char * localVersionCache = "versions/versions.dat";

class ListLoadError : public MMCError
{
public:
	ListLoadError(QString cause) : MMCError(cause) {};
	virtual ~ListLoadError() noexcept
	{
	}
};

MinecraftVersionList::MinecraftVersionList(QObject *parent) : BaseVersionList(parent)
{
	loadBuiltinList();
	loadCachedList();
}

Task *MinecraftVersionList::getLoadTask()
{
	return new MCVListLoadTask(this);
}

bool MinecraftVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr MinecraftVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int MinecraftVersionList::count() const
{
	return m_vlist.count();
}

static bool cmpVersions(BaseVersionPtr first, BaseVersionPtr second)
{
	auto left = std::dynamic_pointer_cast<MinecraftVersion>(first);
	auto right = std::dynamic_pointer_cast<MinecraftVersion>(second);
	return left->m_releaseTime > right->m_releaseTime;
}

void MinecraftVersionList::sortInternal()
{
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
}

void MinecraftVersionList::loadCachedList()
{
	QFile localIndex(localVersionCache);
	if (!localIndex.exists())
	{
		return;
	}
	if (!localIndex.open(QIODevice::ReadOnly))
	{
		// FIXME: this is actually a very bad thing! How do we deal with this?
		QLOG_ERROR() << "The minecraft version cache can't be read.";
		return;
	}
	auto data = localIndex.readAll();
	try
	{
		localIndex.close();
		QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(data);
		if (jsonDoc.isNull())
		{
			throw ListLoadError(tr("Error reading the version list."));
		}
		loadMojangList(jsonDoc, Local);
	}
	catch (MMCError &e)
	{
		// the cache has gone bad for some reason... flush it.
		QLOG_ERROR() << "The minecraft version cache is corrupted. Flushing cache.";
		localIndex.remove();
		return;
	}
	m_hasLocalIndex = true;
}

void MinecraftVersionList::loadBuiltinList()
{
	QLOG_INFO() << "Loading builtin version list.";
	// grab the version list data from internal resources.
	QResource versionList(":/versions/minecraft.json");
	QFile filez(versionList.absoluteFilePath());
	filez.open(QIODevice::ReadOnly);
	auto data = filez.readAll();

	// parse the data as json
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	QJsonObject root = jsonDoc.object();

	// parse all the versions
	for (const auto version : MMCJson::ensureArray(root.value("versions")))
	{
		QJsonObject versionObj = version.toObject();
		QString versionID = versionObj.value("id").toString("");
		QString versionTypeStr = versionObj.value("type").toString("");
		if (versionID.isEmpty() || versionTypeStr.isEmpty())
		{
			QLOG_ERROR() << "Parsed version is missing ID or type";
			continue;
		}

		if (g_VersionFilterData.legacyBlacklist.contains(versionID))
		{
			QLOG_WARN() << "Blacklisted legacy version ignored: " << versionID;
			continue;
		}

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		// Parse the timestamp.
		if (!parse_timestamp(versionObj.value("releaseTime").toString(""),
							 mcVersion->m_releaseTimeString, mcVersion->m_releaseTime))
		{
			QLOG_ERROR() << "Error while parsing version" << versionID
						 << ": invalid version timestamp";
			continue;
		}

		// Get the download URL.
		mcVersion->download_url =
			"http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";

		mcVersion->m_versionSource = Builtin;
		mcVersion->m_type = versionTypeStr;
		mcVersion->m_appletClass = versionObj.value("appletClass").toString("");
		mcVersion->m_mainClass = versionObj.value("mainClass").toString("");
		mcVersion->m_jarChecksum = versionObj.value("checksum").toString("");
		mcVersion->m_processArguments = versionObj.value("processArguments").toString("legacy");
		if (versionObj.contains("+traits"))
		{
			for (auto traitVal : MMCJson::ensureArray(versionObj.value("+traits")))
			{
				mcVersion->m_traits.insert(MMCJson::ensureString(traitVal));
			}
		}
		m_lookup[versionID] = mcVersion;
		m_vlist.append(mcVersion);
	}
}

void MinecraftVersionList::loadMojangList(QJsonDocument jsonDoc, VersionSource source)
{
	QLOG_INFO() << "Loading" << ((source == Remote) ? "remote" : "local") << "version list.";

	if (!jsonDoc.isObject())
	{
		throw ListLoadError(tr("Error parsing version list JSON: jsonDoc is not an object"));
	}

	QJsonObject root = jsonDoc.object();

	try
	{
		QJsonObject latest = MMCJson::ensureObject(root.value("latest"));
		m_latestReleaseID = MMCJson::ensureString(latest.value("release"));
		m_latestSnapshotID = MMCJson::ensureString(latest.value("snapshot"));
	}
	catch (MMCError &err)
	{
		QLOG_ERROR()
			<< tr("Error parsing version list JSON: couldn't determine latest versions");
	}

	// Now, get the array of versions.
	if (!root.value("versions").isArray())
	{
		throw ListLoadError(tr("Error parsing version list JSON: version list object is "
							   "missing 'versions' array"));
	}
	QJsonArray versions = root.value("versions").toArray();

	QList<BaseVersionPtr> tempList;
	for (auto version : versions)
	{
		// Load the version info.
		if (!version.isObject())
		{
			QLOG_ERROR() << "Error while parsing version list : invalid JSON structure";
			continue;
		}

		QJsonObject versionObj = version.toObject();
		QString versionID = versionObj.value("id").toString("");
		if (versionID.isEmpty())
		{
			QLOG_ERROR() << "Error while parsing version : version ID is missing";
			continue;
		}

		if (g_VersionFilterData.legacyBlacklist.contains(versionID))
		{
			QLOG_WARN() << "Blacklisted legacy version ignored: " << versionID;
			continue;
		}

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		if (!parse_timestamp(versionObj.value("releaseTime").toString(""),
							 mcVersion->m_releaseTimeString, mcVersion->m_releaseTime))
		{
			QLOG_ERROR() << "Error while parsing version" << versionID
						 << ": invalid release timestamp";
			continue;
		}
		if (!parse_timestamp(versionObj.value("time").toString(""),
							 mcVersion->m_updateTimeString, mcVersion->m_updateTime))
		{
			QLOG_ERROR() << "Error while parsing version" << versionID
						 << ": invalid update timestamp";
			continue;
		}

		if (mcVersion->m_releaseTime < g_VersionFilterData.legacyCutoffDate)
		{
			continue;
		}

		// depends on where we load the version from -- network request or local file?
		mcVersion->m_versionSource = source;

		QString dlUrl = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";
		mcVersion->download_url = dlUrl;
		QString versionTypeStr = versionObj.value("type").toString("");
		if (versionTypeStr.isEmpty())
		{
			// FIXME: log this somewhere
			continue;
		}
		// OneSix or Legacy. use filter to determine type
		if (versionTypeStr == "release")
		{
		}
		else if (versionTypeStr == "snapshot") // It's a snapshot... yay
		{
		}
		else if (versionTypeStr == "old_alpha")
		{
		}
		else if (versionTypeStr == "old_beta")
		{
		}
		else
		{
			// FIXME: log this somewhere
			continue;
		}
		mcVersion->m_type = versionTypeStr;
		tempList.append(mcVersion);
	}
	updateListData(tempList);
	if(source == Remote)
	{
		m_loaded = true;
	}
}

void MinecraftVersionList::sort()
{
	beginResetModel();
	sortInternal();
	endResetModel();
}

BaseVersionPtr MinecraftVersionList::getLatestStable() const
{
	if(m_lookup.contains(m_latestReleaseID))
		return m_lookup[m_latestReleaseID];
	return BaseVersionPtr();
}

void MinecraftVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	for (auto version : versions)
	{
		auto descr = version->descriptor();

		if (!m_lookup.contains(descr))
		{
			m_lookup[version->descriptor()] = version;
			m_vlist.append(version);
			continue;
		}
		auto orig = std::dynamic_pointer_cast<MinecraftVersion>(m_lookup[descr]);
		auto added = std::dynamic_pointer_cast<MinecraftVersion>(version);
		// updateListData is called after Mojang list loads. those can be local or remote
		// remote comes always after local
		// any other options are ignored
		if (orig->m_versionSource != Local || added->m_versionSource != Remote)
		{
			continue;
		}
		// is it actually an update?
		if (orig->m_updateTime >= added->m_updateTime)
		{
			// nope.
			continue;
		}
		// alright, it's an update. put it inside the original, for further processing.
		orig->upstreamUpdate = added;
	}
	sortInternal();
	endResetModel();
}

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

MCVListLoadTask::MCVListLoadTask(MinecraftVersionList *vlist)
{
	m_list = vlist;
	m_currentStable = NULL;
	vlistReply = nullptr;
}

void MCVListLoadTask::executeTask()
{
	setStatus(tr("Loading instance version list..."));
	auto worker = MMC->qnam();
	vlistReply = worker->get(QNetworkRequest(
		QUrl("http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + "versions.json")));
	connect(vlistReply, SIGNAL(finished()), this, SLOT(list_downloaded()));
}

void MCVListLoadTask::list_downloaded()
{
	if (vlistReply->error() != QNetworkReply::NoError)
	{
		vlistReply->deleteLater();
		emitFailed("Failed to load Minecraft main version list" + vlistReply->errorString());
		return;
	}

	auto data = vlistReply->readAll();
	vlistReply->deleteLater();
	try
	{
		QJsonParseError jsonError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			throw ListLoadError(
				tr("Error parsing version list JSON: %1").arg(jsonError.errorString()));
		}
		m_list->loadMojangList(jsonDoc, Remote);
	}
	catch (MMCError &e)
	{
		emitFailed(e.cause());
		return;
	}

	emitSucceeded();
	return;
}

MCVListVersionUpdateTask::MCVListVersionUpdateTask(MinecraftVersionList *vlist,
												   QString updatedVersion)
	: Task()
{
	m_list = vlist;
	versionToUpdate = updatedVersion;
}

void MCVListVersionUpdateTask::executeTask()
{
	QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionToUpdate + "/" +
					 versionToUpdate + ".json";
	auto job = new NetJob("Version index");
	job->addNetAction(ByteArrayDownload::make(QUrl(urlstr)));
	specificVersionDownloadJob.reset(job);
	connect(specificVersionDownloadJob.get(), SIGNAL(succeeded()), SLOT(json_downloaded()));
	connect(specificVersionDownloadJob.get(), SIGNAL(failed(QString)), SIGNAL(failed(QString)));
	connect(specificVersionDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
	specificVersionDownloadJob->start();
}

void MCVListVersionUpdateTask::json_downloaded()
{
	NetActionPtr DlJob = specificVersionDownloadJob->first();
	auto data = std::dynamic_pointer_cast<ByteArrayDownload>(DlJob)->m_data;
	specificVersionDownloadJob.reset();

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed(tr("The download version file is not valid."));
		return;
	}
	VersionFilePtr file;
	try
	{
		file = VersionFile::fromJson(jsonDoc, "net.minecraft.json", false);
	}
	catch (MMCError &e)
	{
		emitFailed(tr("Couldn't process version file: %1").arg(e.cause()));
		return;
	}
	QList<RawLibraryPtr> filteredLibs;
	QList<RawLibraryPtr> lwjglLibs;
	QSet<QString> lwjglFilter = {
		"net.java.jinput:jinput",	 "net.java.jinput:jinput-platform",
		"net.java.jutils:jutils",	 "org.lwjgl.lwjgl:lwjgl",
		"org.lwjgl.lwjgl:lwjgl_util", "org.lwjgl.lwjgl:lwjgl-platform"};
	for (auto lib : file->overwriteLibs)
	{
		if (lwjglFilter.contains(lib->fullname()))
		{
			lwjglLibs.append(lib);
		}
		else
		{
			filteredLibs.append(lib);
		}
	}
	file->overwriteLibs = filteredLibs;

	// TODO: recognize and add LWJGL versions here.

	file->fileId = "net.minecraft";

	// now dump the file to disk
	auto doc = file->toJson(false);
	auto newdata = doc.toBinaryData();
	QLOG_INFO() << newdata;
	QString targetPath = "versions/" + versionToUpdate + "/" + versionToUpdate + ".dat";
	ensureFilePathExists(targetPath);
	QSaveFile vfile1(targetPath);
	if (!vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly))
	{
		emitFailed(tr("Can't open %1 for writing.").arg(targetPath));
		return;
	}
	qint64 actual = 0;
	if ((actual = vfile1.write(newdata)) != newdata.size())
	{
		emitFailed(tr("Failed to write into %1. Written %2 out of %3.")
					   .arg(targetPath)
					   .arg(actual)
					   .arg(newdata.size()));
		return;
	}
	if (!vfile1.commit())
	{
		emitFailed(tr("Can't commit changes to %1").arg(targetPath));
		return;
	}

	m_list->finalizeUpdate(versionToUpdate);
	emitSucceeded();
}

std::shared_ptr<Task> MinecraftVersionList::createUpdateTask(QString version)
{
	return std::shared_ptr<Task>(new MCVListVersionUpdateTask(this, version));
}

void MinecraftVersionList::saveCachedList()
{
	// FIXME: throw.
	if (!ensureFilePathExists(localVersionCache))
		return;
	QSaveFile tfile(localVersionCache);
	if (!tfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;
	QJsonObject toplevel;
	QJsonArray entriesArr;
	for (auto version : m_vlist)
	{
		auto mcversion = std::dynamic_pointer_cast<MinecraftVersion>(version);
		// do not save the remote versions.
		if (mcversion->m_versionSource != Local)
			continue;
		QJsonObject entryObj;

		entryObj.insert("id", mcversion->descriptor());
		entryObj.insert("time", mcversion->m_updateTimeString);
		entryObj.insert("releaseTime", mcversion->m_releaseTimeString);
		entryObj.insert("type", mcversion->m_type);
		entriesArr.append(entryObj);
	}
	toplevel.insert("versions", entriesArr);
	
	{
		bool someLatest = false;
		QJsonObject latestObj;
		if(!m_latestReleaseID.isNull())
		{
			latestObj.insert("release", m_latestReleaseID);
			someLatest = true;
		}
		if(!m_latestSnapshotID.isNull())
		{
			latestObj.insert("snapshot", m_latestSnapshotID);
			someLatest = true;
		}
		if(someLatest)
		{
			toplevel.insert("latest", latestObj);
		}
	}
	
	QJsonDocument doc(toplevel);
	QByteArray jsonData = doc.toBinaryData();
	qint64 result = tfile.write(jsonData);
	if (result == -1)
		return;
	if (result != jsonData.size())
		return;
	tfile.commit();
}

void MinecraftVersionList::finalizeUpdate(QString version)
{
	int idx = -1;
	for (int i = 0; i < m_vlist.size(); i++)
	{
		if (version == m_vlist[i]->descriptor())
		{
			idx = i;
			break;
		}
	}
	if (idx == -1)
	{
		return;
	}

	auto updatedVersion = std::dynamic_pointer_cast<MinecraftVersion>(m_vlist[idx]);

	if (updatedVersion->m_versionSource == Builtin)
		return;

	if (updatedVersion->upstreamUpdate)
	{
		auto updatedWith = updatedVersion->upstreamUpdate;
		updatedWith->m_versionSource = Local;
		m_vlist[idx] = updatedWith;
		m_lookup[version] = updatedWith;
	}
	else
	{
		updatedVersion->m_versionSource = Local;
	}

	dataChanged(index(idx), index(idx));

	saveCachedList();
}
