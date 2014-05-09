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

#include "MinecraftVersionList.h"
#include "MultiMC.h"
#include "logic/net/URLConstants.h"
#include "logic/MMCJson.h"
#include "ParseUtils.h"

#include <QtXml>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

#include <QtAlgorithms>

#include <QtNetwork>

MinecraftVersionList::MinecraftVersionList(QObject *parent) : BaseVersionList(parent)
{
	loadBuiltinList();
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

void MinecraftVersionList::loadBuiltinList()
{
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

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		// Parse the timestamp.
		try
		{
			parse_timestamp(versionObj.value("releaseTime").toString(""),
							mcVersion->m_releaseTimeString, mcVersion->m_releaseTime);
		}
		catch (MMCError &e)
		{
			QLOG_ERROR() << "Error while parsing version" << versionID << ":" << e.cause();
			continue;
		}

		// Get the download URL.
		mcVersion->download_url =
			"http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";

		mcVersion->m_versionSource = MinecraftVersion::Builtin;
		mcVersion->m_appletClass = versionObj.value("appletClass").toString("");
		mcVersion->m_mainClass = versionObj.value("mainClass").toString("");
		mcVersion->m_processArguments = versionObj.value("processArguments").toString("legacy");
		if (versionObj.contains("+traits"))
		{
			for (auto traitVal : MMCJson::ensureArray(versionObj.value("+traits")))
			{
				mcVersion->m_traits.insert(MMCJson::ensureString(traitVal));
			}
		}
		m_vlist.append(mcVersion);
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
	for (int i = 0; i < m_vlist.length(); i++)
	{
		auto ver = std::dynamic_pointer_cast<MinecraftVersion>(m_vlist.at(i));
		if (ver->is_latest && !ver->is_snapshot)
		{
			return m_vlist.at(i);
		}
	}
	return BaseVersionPtr();
}

void MinecraftVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	for (auto version : versions)
	{
		auto descr = version->descriptor();
		for (auto builtin_v : m_vlist)
		{
			if (descr == builtin_v->descriptor())
			{
				goto SKIP_THIS_ONE;
			}
		}
		m_vlist.append(version);
	SKIP_THIS_ONE:
	{
	}
	}
	m_loaded = true;
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

MCVListLoadTask::~MCVListLoadTask()
{
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

	auto foo = vlistReply->readAll();
	QJsonParseError jsonError;
	QLOG_INFO() << foo;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(foo, &jsonError);
	vlistReply->deleteLater();

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

	QJsonObject root = jsonDoc.object();

	QString latestReleaseID = "INVALID";
	QString latestSnapshotID = "INVALID";
	try
	{
		QJsonObject latest = MMCJson::ensureObject(root.value("latest"));
		latestReleaseID = MMCJson::ensureString(latest.value("release"));
		latestSnapshotID = MMCJson::ensureString(latest.value("snapshot"));
	}
	catch (MMCError &err)
	{
		QLOG_ERROR()
			<< tr("Error parsing version list JSON: couldn't determine latest versions");
	}

	// Now, get the array of versions.
	if (!root.value("versions").isArray())
	{
		emitFailed(
			"Error parsing version list JSON: version list object is missing 'versions' array");
		return;
	}
	QJsonArray versions = root.value("versions").toArray();

	QList<BaseVersionPtr> tempList;
	for (auto version : versions)
	{
		bool is_snapshot = false;
		bool is_latest = false;

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
		// Get the download URL.
		QString dlUrl = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		try
		{
			// Parse the timestamps.
			parse_timestamp(versionObj.value("releaseTime").toString(""),
							mcVersion->m_releaseTimeString, mcVersion->m_releaseTime);

			parse_timestamp(versionObj.value("time").toString(""),
							mcVersion->m_updateTimeString, mcVersion->m_updateTime);
		}
		catch (MMCError &e)
		{
			QLOG_ERROR() << "Error while parsing version" << versionID << ":" << e.cause();
			continue;
		}

		mcVersion->m_versionSource = MinecraftVersion::Builtin;
		mcVersion->download_url = dlUrl;
		{
			QString versionTypeStr = versionObj.value("type").toString("");
			if (versionTypeStr.isEmpty())
			{
				// FIXME: log this somewhere
				continue;
			}
			// OneSix or Legacy. use filter to determine type
			if (versionTypeStr == "release")
			{
				is_latest = (versionID == latestReleaseID);
				is_snapshot = false;
			}
			else if (versionTypeStr == "snapshot") // It's a snapshot... yay
			{
				is_latest = (versionID == latestSnapshotID);
				is_snapshot = true;
			}
			else if (versionTypeStr == "old_alpha")
			{
				is_latest = false;
				is_snapshot = false;
			}
			else if (versionTypeStr == "old_beta")
			{
				is_latest = false;
				is_snapshot = false;
			}
			else
			{
				// FIXME: log this somewhere
				continue;
			}
			mcVersion->m_type = versionTypeStr;
			mcVersion->is_latest = is_latest;
			mcVersion->is_snapshot = is_snapshot;
		}
		tempList.append(mcVersion);
	}
	m_list->updateListData(tempList);

	emitSucceeded();
	return;
}
