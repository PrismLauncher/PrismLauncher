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

#include "include/minecraftversionlist.h"

#include <QDebug>

#include <QtXml>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

#include <QtAlgorithms>

#include <QtNetwork>

#include "netutils.h"

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
}

MCVListLoadTask::~MCVListLoadTask()
{
//	delete netMgr;
}

void MCVListLoadTask::executeTask()
{
	setSubStatus();
	
	QNetworkAccessManager networkMgr;
	netMgr = &networkMgr;
	
	if (!loadFromVList())
	{
		qDebug() << "Failed to load from Mojang version list.";
	}
	if (!loadFromAssets())
	{
		qDebug() << "Failed to load assets version list.";
	}
	if (!loadMCNostalgia())
	{
		qDebug() << "Failed to load MCNostalgia version list.";
	}
	finalize();
}

void MCVListLoadTask::setSubStatus(const QString msg)
{
	if (msg.isEmpty())
		setStatus("Loading instance version list...");
	else
		setStatus("Loading instance version list: " + msg);
}

bool MCVListLoadTask::loadFromVList()
{
	QNetworkReply *vlistReply = netMgr->get(QNetworkRequest(QUrl(QString(MCVLIST_URLBASE) + 
																 "versions.json")));
	NetUtils::waitForNetRequest(vlistReply);
	
	switch (vlistReply->error())
	{
	case QNetworkReply::NoError:
	{
		QJsonParseError jsonError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(vlistReply->readAll(), &jsonError);
		
		if (jsonError.error == QJsonParseError::NoError)
		{
			Q_ASSERT_X(jsonDoc.isObject(), "loadFromVList", "jsonDoc is not an object");
			
			QJsonObject root = jsonDoc.object();
			
			// Get the ID of the latest release and the latest snapshot.
			Q_ASSERT_X(root.value("latest").isObject(), "loadFromVList", 
					   "version list is missing 'latest' object");
			QJsonObject latest = root.value("latest").toObject();
			
			QString latestReleaseID = latest.value("release").toString("");
			QString latestSnapshotID = latest.value("snapshot").toString("");
			Q_ASSERT_X(!latestReleaseID.isEmpty(), "loadFromVList", "latest release field is missing");
			Q_ASSERT_X(!latestSnapshotID.isEmpty(), "loadFromVList", "latest snapshot field is missing");
			
			// Now, get the array of versions.
			Q_ASSERT_X(root.value("versions").isArray(), "loadFromVList", 
					   "version list object is missing 'versions' array");
			QJsonArray versions = root.value("versions").toArray();
			
			for (int i = 0; i < versions.count(); i++)
			{
				// Load the version info.
				Q_ASSERT_X(versions[i].isObject(), "loadFromVList",
						   QString("in versions array, index %1 is not an object").
						   arg(i).toUtf8());
				QJsonObject version = versions[i].toObject();
				
				QString versionID = version.value("id").toString("");
				QString versionTimeStr = version.value("releaseTime").toString("");
				QString versionTypeStr = version.value("type").toString("");
				
				Q_ASSERT_X(!versionID.isEmpty(), "loadFromVList", 
						   QString("in versions array, index %1's \"id\" field is not a valid string").
						   arg(i).toUtf8());
				Q_ASSERT_X(!versionTimeStr.isEmpty(), "loadFromVList",
						   QString("in versions array, index %1's \"time\" field is not a valid string").
						   arg(i).toUtf8());
				Q_ASSERT_X(!versionTypeStr.isEmpty(), "loadFromVList", 
						   QString("in versions array, index %1's \"type\" field is not a valid string").
						   arg(i).toUtf8());
				
				
				// Now, process that info and add the version to the list.
				
				// Parse the timestamp.
				QDateTime versionTime = timeFromS3Time(versionTimeStr);
				
				Q_ASSERT_X(versionTime.isValid(), "loadFromVList",
						   QString("in versions array, index %1's timestamp failed to parse").
						   arg(i).toUtf8());
				
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
				else
				{
					versionType = MinecraftVersion::Snapshot;
				}
				
				// Get the download URL.
				QString dlUrl = QString(MCVLIST_URLBASE) + versionID + "/";
				
				
				// Now, we construct the version object and add it to the list.
				MinecraftVersion *mcVersion = new MinecraftVersion(
							versionID, versionID, versionTime.toMSecsSinceEpoch(),
							dlUrl, "");
				mcVersion->setIsForNewLauncher(true);
				mcVersion->setVersionType(versionType);
				tempList.append(mcVersion);
			}
		}
		else
		{
			qDebug() << "Error parsing version list JSON:" << jsonError.errorString();
		}
		
		break;
	}
		
	default:
		// TODO: Network error handling.
		qDebug() << "Failed to load Minecraft main version list" << vlistReply->errorString();
		break;
	}
	
	return true;
}

bool MCVListLoadTask::loadFromAssets()
{
	setSubStatus("Loading versions from assets.minecraft.net...");
	
	bool succeeded = false;
	
	QNetworkReply *assetsReply = netMgr->get(QNetworkRequest(QUrl(ASSETS_URLBASE)));
	NetUtils::waitForNetRequest(assetsReply);
	
	switch (assetsReply->error())
	{
	case QNetworkReply::NoError:
	{
		// Get the XML string.
		QString xmlString = assetsReply->readAll();
		
		QString xmlErrorMsg;
		
		QDomDocument doc;
		if (!doc.setContent(xmlString, false, &xmlErrorMsg))
		{
			// TODO: Display error message to the user.
			qDebug() << "Failed to process assets.minecraft.net. XML error:" <<
						xmlErrorMsg << xmlString;
		}
		
		QDomNodeList contents = doc.elementsByTagName("Contents");
		
		QRegExp mcRegex("/minecraft.jar$");
		QRegExp snapshotRegex("[0-9][0-9]w[0-9][0-9][a-z]|pre|rc");
		
		for (int i = 0; i < contents.length(); i++)
		{
			QDomElement element = contents.at(i).toElement();
			
			if (element.isNull())
				continue;
			
			QDomElement keyElement = getDomElementByTagName(element, "Key");
			QDomElement lastmodElement = getDomElementByTagName(element, "LastModified");
			QDomElement etagElement = getDomElementByTagName(element, "ETag");
			
			if (keyElement.isNull() || lastmodElement.isNull() || etagElement.isNull())
				continue;
			
			QString key = keyElement.text();
			QString lastModStr = lastmodElement.text();
			QString etagStr = etagElement.text();
			
			if (!key.contains(mcRegex))
				continue;
			
			QString versionDirName = key.left(key.length() - 14);
			QString dlUrl = QString("http://assets.minecraft.net/%1/").arg(versionDirName);
			
			QString versionName = versionDirName.replace("_", ".");
			
			QDateTime versionTimestamp = timeFromS3Time(lastModStr);
			if (!versionTimestamp.isValid())
			{
				qDebug(QString("Failed to parse timestamp for version %1 %2").
					   arg(versionName, lastModStr).toUtf8());
				versionTimestamp = QDateTime::currentDateTime();
			}
			
			if (m_currentStable)
			{
				{
					bool older = versionTimestamp.toMSecsSinceEpoch() < m_currentStable->timestamp();
					bool newer = versionTimestamp.toMSecsSinceEpoch() > m_currentStable->timestamp();
					bool isSnapshot = versionName.contains(snapshotRegex);
					
					MinecraftVersion *version = new MinecraftVersion(
								versionName, versionName, 
								versionTimestamp.toMSecsSinceEpoch(),
								dlUrl, etagStr);
					
					if (newer)
					{
						version->setVersionType(MinecraftVersion::Snapshot);
					}
					else if (older && isSnapshot)
					{
						version->setVersionType(MinecraftVersion::OldSnapshot);
					}
					else if (older)
					{
						version->setVersionType(MinecraftVersion::Stable);
					}
					else
					{
						// Shouldn't happen, but just in case...
						version->setVersionType(MinecraftVersion::CurrentStable);
					}
					
					assetsList.push_back(version);
				}
			}
			else // If there isn't a current stable version.
			{
				bool isSnapshot = versionName.contains(snapshotRegex);
				
				MinecraftVersion *version = new MinecraftVersion(
							versionName, versionName, 
							versionTimestamp.toMSecsSinceEpoch(),
							dlUrl, etagStr);
				version->setVersionType(isSnapshot? MinecraftVersion::Snapshot :
													MinecraftVersion::Stable);
				assetsList.push_back(version);
			}
		}
		
		setSubStatus("Loaded assets.minecraft.net");
		succeeded = true;
		break;
	}
		
	default:
		// TODO: Network error handling.
		qDebug() << "Failed to load assets.minecraft.net" << assetsReply->errorString();
		break;
	}
	
	processedAssetsReply = true;
	updateStuff();
	return succeeded;
}

bool MCVListLoadTask::loadMCNostalgia()
{
	QNetworkReply *mcnReply = netMgr->get(QNetworkRequest(QUrl(QString(MCN_URLBASE) + "?pversion=1&list=True")));
	NetUtils::waitForNetRequest(mcnReply);
	return true;
}

bool MCVListLoadTask::finalize()
{
	// First, we need to do some cleanup. We loaded assets versions into assetsList,
	// MCNostalgia versions into mcnList and all the others into tempList. MCNostalgia 
	// provides some versions that are on assets.minecraft.net and we want to ignore 
	// those, so we remove and delete them from mcnList. assets.minecraft.net also provides
	// versions that are on Mojang's version list and we want to ignore those as well.
	
	// To start, we get a list of the descriptors in tmpList.
	QStringList tlistDescriptors;
	for (int i = 0; i < tempList.count(); i++)
		tlistDescriptors.append(tempList.at(i)->descriptor());
	
	// Now, we go through our assets version list and remove anything with
	// a descriptor that matches one we already have in tempList.
	for (int i = 0; i < assetsList.count(); i++)
		if (tlistDescriptors.contains(assetsList.at(i)->descriptor()))
			delete assetsList.takeAt(i--); // We need to decrement here because we're removing an item.
	
	// We also need to rebuild the list of descriptors.
	tlistDescriptors.clear();
	for (int i = 0; i < tempList.count(); i++)
		tlistDescriptors.append(tempList.at(i)->descriptor());
	
	// Next, we go through our MCNostalgia version list and do the same thing.
	for (int i = 0; i < mcnList.count(); i++)
		if (tlistDescriptors.contains(mcnList.at(i)->descriptor()))
			delete mcnList.takeAt(i--); // We need to decrement here because we're removing an item.
	
	// Now that the duplicates are gone, we need to merge the lists. This is
	// simple enough.
	tempList.append(assetsList);
	tempList.append(mcnList);
	
	// We're done with these lists now, but the items have been moved over to 
	// tempList, so we don't need to delete them yet.
	
	// Now, we invoke the updateListData slot on the GUI thread. This will copy all
	// the versions we loaded and set their parents to the version list.
	// Then, it will swap the new list with the old one and free the old list's memory.
	QMetaObject::invokeMethod(m_list, "updateListData", Qt::BlockingQueuedConnection, 
							  Q_ARG(QList<InstVersion*>, tempList));
	
	// Once that's finished, we can delete the versions in our temp list.
	while (!tempList.isEmpty())
		delete tempList.takeFirst();
	
#ifdef PRINT_VERSIONS
	m_list->printToStdOut();
#endif
	return true;
}

void MCVListLoadTask::updateStuff()
{
	const int totalReqs = 3;
	int reqsComplete = 0;
	
	if (processedMCVListReply)
		reqsComplete++;
	if (processedAssetsReply)
		reqsComplete++;
	if (processedMCNReply)
		reqsComplete++;
	
	calcProgress(reqsComplete, totalReqs);
	
	if (reqsComplete >= totalReqs)
	{
		quit();
	}
}
