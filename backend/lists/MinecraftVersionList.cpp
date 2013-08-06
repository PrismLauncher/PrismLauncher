/* Copyright 2013 Andrew Okin
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
#include <net/NetWorker.h>

#include <QDebug>

#include <QtXml>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

#include <QtAlgorithms>

#include <QtNetwork>

#define MCVLIST_URLBASE "http://s3.amazonaws.com/Minecraft.Download/versions/"
#define ASSETS_URLBASE "http://assets.minecraft.net/"
#define MCN_URLBASE "http://sonicrules.org/mcnweb.py"

MinecraftVersionList mcVList;

MinecraftVersionList::MinecraftVersionList(QObject *parent) :
	InstVersionList(parent)
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

const InstVersion *MinecraftVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int MinecraftVersionList::count() const
{
	return m_vlist.count();
}

void MinecraftVersionList::printToStdOut() const
{
	qDebug() << "---------------- Version List ----------------";
	
	for (int i = 0; i < m_vlist.count(); i++)
	{
		MinecraftVersion *version = qobject_cast<MinecraftVersion *>(m_vlist.at(i));
		
		if (!version)
			continue;
		
		qDebug() << "Version " << version->name();
		qDebug() << "\tDownload: " << version->downloadURL();
		qDebug() << "\tTimestamp: " << version->timestamp();
		qDebug() << "\tType: " << version->typeName();
		qDebug() << "----------------------------------------------";
	}
}

bool cmpVersions(const InstVersion *first, const InstVersion *second)
{
	return !first->isLessThan(*second);
}

void MinecraftVersionList::sort()
{
	beginResetModel();
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}

InstVersion *MinecraftVersionList::getLatestStable() const
{
	for (int i = 0; i < m_vlist.length(); i++)
	{
		if (((MinecraftVersion *)m_vlist.at(i))->versionType() == MinecraftVersion::CurrentStable)
		{
			return m_vlist.at(i);
		}
	}
	return NULL;
}

MinecraftVersionList &MinecraftVersionList::getMainList()
{
	return mcVList;
}

void MinecraftVersionList::updateListData(QList<InstVersion *> versions)
{
	// First, we populate a temporary list with the copies of the versions.
	QList<InstVersion *> tempList;
	for (int i = 0; i < versions.length(); i++)
	{
		InstVersion *version = versions[i]->copyVersion(this);
		Q_ASSERT(version != NULL);
		tempList.append(version);
	}
	
	// Now we swap the temporary list into the actual version list.
	// This applies our changes to the version list immediately and still gives us 
	// access to the old version list so that we can delete the objects in it and 
	// free their memory. By doing this, we cause the version list to update as 
	// quickly as possible.
	beginResetModel();
	m_vlist.swap(tempList);
	m_loaded = true;
	endResetModel();
	
	// We called swap, so all the data that was in the version list previously is now in 
	// tempList (and vice-versa). Now we just free the memory.
	while (!tempList.isEmpty())
		delete tempList.takeFirst();
	
	// NOW SORT!!
	sort();
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
	setStatus("Loading instance version list...");
	auto & worker = NetWorker::spawn();
	vlistReply = worker.get(QNetworkRequest(QUrl(QString(MCVLIST_URLBASE) + "versions.json")));
	connect(vlistReply, SIGNAL(finished()), this, SLOT(list_downloaded()));
}


void MCVListLoadTask::list_downloaded()
{
	if(vlistReply->error() != QNetworkReply::QNetworkReply::NoError)
	{
		qDebug() << "Failed to load Minecraft main version list" << vlistReply->errorString();
		vlistReply->deleteLater();
		emitEnded();
		return;
	}
	
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(vlistReply->readAll(), &jsonError);
	vlistReply->deleteLater();
	
	if (jsonError.error != QJsonParseError::NoError)
	{
		qDebug() << "Error parsing version list JSON:" << jsonError.errorString();
		emitEnded();
		return;
	}

	if(!jsonDoc.isObject())
	{
		qDebug() << "Error parsing version list JSON: " << "jsonDoc is not an object";
		emitEnded();
		return;
	}
	
	QJsonObject root = jsonDoc.object();
	
	// Get the ID of the latest release and the latest snapshot.
	if(!root.value("latest").isObject())
	{
		qDebug() << "Error parsing version list JSON: " << "version list is missing 'latest' object";
		emitEnded();
		return;
	}
	
	QJsonObject latest = root.value("latest").toObject();
	
	QString latestReleaseID = latest.value("release").toString("");
	QString latestSnapshotID = latest.value("snapshot").toString("");
	if(latestReleaseID.isEmpty())
	{
		qDebug() << "Error parsing version list JSON: " << "latest release field is missing";
		emitEnded();
		return;
	}
	if(latestSnapshotID.isEmpty())
	{
		qDebug() << "Error parsing version list JSON: " << "latest snapshot field is missing";
		emitEnded();
		return;
	}

	// Now, get the array of versions.
	if(!root.value("versions").isArray())
	{
		qDebug() << "Error parsing version list JSON: " << "version list object is missing 'versions' array";
		emitEnded();
		return;
	}
	QJsonArray versions = root.value("versions").toArray();
	
	for (int i = 0; i < versions.count(); i++)
	{
		// Load the version info.
		if(!versions[i].isObject())
		{
			//FIXME: log this somewhere
			continue;
		}
		QJsonObject version = versions[i].toObject();
		QString versionID = version.value("id").toString("");
		QString versionTimeStr = version.value("releaseTime").toString("");
		QString versionTypeStr = version.value("type").toString("");
		if(versionID.isEmpty() || versionTimeStr.isEmpty() || versionTypeStr.isEmpty())
		{
			//FIXME: log this somewhere
			continue;
		}
		
		// Parse the timestamp.
		QDateTime versionTime = timeFromS3Time(versionTimeStr);
		if(!versionTime.isValid())
		{
			//FIXME: log this somewhere
			continue;
		}
		
		// Parse the type.
		MinecraftVersion::VersionType versionType;
		if (versionTypeStr == "release")
		{
			// Check if this version is the current stable version.
			if (versionID == latestReleaseID)
				versionType = MinecraftVersion::CurrentStable;
			else
				versionType = MinecraftVersion::Stable;
		}
		else if(versionTypeStr == "snapshot")
		{
			versionType = MinecraftVersion::Snapshot;
		}
		else if(versionTypeStr == "old_beta" || versionTypeStr == "old_alpha")
		{
			versionType = MinecraftVersion::Nostalgia;
		}
		else
		{
			//FIXME: log this somewhere
			continue;
		}
		
		//FIXME: detect if snapshots are old or not
		
		// Get the download URL.
		QString dlUrl = QString(MCVLIST_URLBASE) + versionID + "/";
		
		// Now, we construct the version object and add it to the list.
		MinecraftVersion *mcVersion = new MinecraftVersion(versionID, versionID, versionTime.toMSecsSinceEpoch(),dlUrl, "");
		mcVersion->setVersionType(versionType);
		tempList.append(mcVersion);
	}
	m_list->updateListData(tempList);
	
	// Once that's finished, we can delete the versions in our temp list.
	while (!tempList.isEmpty())
		delete tempList.takeFirst();
	
#ifdef PRINT_VERSIONS
	m_list->printToStdOut();
#endif
	emitEnded();
	return;
}

// FIXME: we should have a local cache of the version list and a local cache of version data
bool MCVListLoadTask::loadFromVList()
{
}
