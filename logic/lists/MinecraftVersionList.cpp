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
	return left->timestamp > right->timestamp;
}

void MinecraftVersionList::sortInternal()
{
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
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
	m_vlist = versions;
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

inline QDateTime timeFromS3Time(QString str)
{
	return QDateTime::fromString(str, Qt::ISODate);
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
	vlistReply = worker->get(QNetworkRequest(QUrl("http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + "versions.json")));
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

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(vlistReply->readAll(), &jsonError);
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

	// Get the ID of the latest release and the latest snapshot.
	if (!root.value("latest").isObject())
	{
		emitFailed("Error parsing version list JSON: version list is missing 'latest' object");
		return;
	}

	QJsonObject latest = root.value("latest").toObject();

	QString latestReleaseID = latest.value("release").toString("");
	QString latestSnapshotID = latest.value("snapshot").toString("");
	if (latestReleaseID.isEmpty())
	{
		emitFailed("Error parsing version list JSON: latest release field is missing");
		return;
	}
	if (latestSnapshotID.isEmpty())
	{
		emitFailed("Error parsing version list JSON: latest snapshot field is missing");
		return;
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
	for (int i = 0; i < versions.count(); i++)
	{
		bool is_snapshot = false;
		bool is_latest = false;
		bool legacyLaunch = false;

		// Load the version info.
		if (!versions[i].isObject())
		{
			// FIXME: log this somewhere
			continue;
		}
		QJsonObject version = versions[i].toObject();
		QString versionID = version.value("id").toString("");
		QString versionTimeStr = version.value("releaseTime").toString("");
		QString versionTypeStr = version.value("type").toString("");
		if (versionID.isEmpty() || versionTimeStr.isEmpty() || versionTypeStr.isEmpty())
		{
			// FIXME: log this somewhere
			continue;
		}

		// Parse the timestamp.
		QDateTime versionTime = timeFromS3Time(versionTimeStr);
		if (!versionTime.isValid())
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
		// Get the download URL.
		QString dlUrl = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;
		mcVersion->timestamp = versionTime.toMSecsSinceEpoch();
		mcVersion->download_url = dlUrl;
		mcVersion->is_latest = is_latest;
		mcVersion->is_snapshot = is_snapshot;
		if(legacyLaunch)
			mcVersion->features.insert("legacy");
		tempList.append(mcVersion);
	}
	m_list->updateListData(tempList);

	emitSucceeded();
	return;
}
