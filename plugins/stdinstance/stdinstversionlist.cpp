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

#include "stdinstversionlist.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QtXml/QDomDocument>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

#include <QDateTime>
#include <QMap>
#include <QMapIterator>
#include <QStringList>
#include <QUrl>

#include <QRegExp>

#include <QDebug>

#include <instversion.h>

#include "stdinstversion.h"

#define MCDL_URLBASE "http://assets.minecraft.net/"
#define ASSETS_URLBASE "http://s3.amazonaws.com/MinecraftDownload/"
#define MCN_URLBASE "http://sonicrules.org/mcnweb.py"

// When this is defined, prints the entire version list to qDebug() after loading.
//#define PRINT_VERSIONS


StdInstVersionList vList;

StdInstVersionList::StdInstVersionList(QObject *parent) :
	InstVersionList(parent)
{
	loaded = false;
}

Task *StdInstVersionList::getLoadTask()
{
	return new StdInstVListLoadTask(this);
}

bool StdInstVersionList::isLoaded()
{
	return loaded;
}

const InstVersion *StdInstVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int StdInstVersionList::count() const
{
	return m_vlist.count();
}

void StdInstVersionList::printToStdOut()
{
	qDebug() << "---------------- Version List ----------------";
	
	for (int i = 0; i < m_vlist.count(); i++)
	{
		StdInstVersion *version = qobject_cast<StdInstVersion *>(m_vlist.at(i));
		
		if (!version)
			continue;
		
		qDebug() << "Version " << version->name();
		qDebug() << "\tDownload: " << version->downloadURL();
		qDebug() << "\tTimestamp: " << version->timestamp();
		qDebug() << "\tType: " << version->typeName();
		qDebug() << "----------------------------------------------";
	}
}


StdInstVListLoadTask::StdInstVListLoadTask(StdInstVersionList *vlist) :
	Task(vlist)
{
	m_list = vlist;
	processedMCDLReply = false;
	processedAssetsReply = false;
	processedMCNReply = false;
	
	currentStable = NULL;
	foundCurrentInAssets = false;
}

void StdInstVListLoadTask::executeTask()
{
	setSubStatus();
	
	// Initialize the network access manager.
	QNetworkAccessManager netMgr;
	
	mcdlReply = netMgr.get(QNetworkRequest(QUrl(ASSETS_URLBASE)));
	assetsReply = netMgr.get(QNetworkRequest(QUrl(MCDL_URLBASE)));
	mcnReply = netMgr.get(QNetworkRequest(QUrl(QString(MCN_URLBASE) + "?pversion=1&list=True")));
	
	connect(mcdlReply, SIGNAL(finished()),
			SLOT(processMCDLReply()));
	connect(mcnReply, SIGNAL(finished()),
			SLOT(processMCNReply()));
	
	exec();
	finalize();
}

void StdInstVListLoadTask::finalize()
{
	// First, we need to do some cleanup. We loaded MCNostalgia versions into 
	// mcnList and all the others into tempList. MCNostalgia provides some versions
	// that are on assets.minecraft.net and we want to ignore those, so we remove
	// and delete them from mcnList.
	
	// To start, we get a list of the descriptors in tmpList.
	QStringList tlistDescriptors;
	for (int i = 0; i < tempList.count(); i++)
		tlistDescriptors.append(tempList.at(i)->descriptor());
	
	// Now, we go through our MCNostalgia version list and remove anything with
	// a descriptor that matches one we already have in tempList.
	// We'll need a list of items we're going to remove.
	for (int i = 0; i < mcnList.count(); i++)
		if (tlistDescriptors.contains(mcnList.at(i)->descriptor()))
			delete mcnList.takeAt(i--); // We need to decrement here because we're removing an item.
	
	// Now that the duplicates are gone, we need to merge the two lists. This is
	// simple enough.
	tempList.append(mcnList);
	
	// We're done with mcnList now, but the items have been moved over to 
	// tempList, so we don't need to delete them.
	
	// Now we swap the list we loaded into the actual version list.
	// This applies our changes to the version list immediately and still gives us 
	// access to the old list so that we can delete the objects in it and free their memory.
	// By doing this, we cause the version list to update as quickly as possible.
	m_list->beginResetModel();
	m_list->m_vlist.swap(tempList);
	m_list->endResetModel();
	
	m_list->loaded = true;
	
	// We called swap, so all the data that was in the version list previously is now in 
	// tempList (and vice-versa). Now we just free the memory.
	while (!tempList.isEmpty())
		delete tempList.takeFirst();
	
#ifdef PRINT_VERSIONS
	m_list->printToStdOut();
#endif
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
    const QString fmt("yyyy-MM-dd'T'HH:mm:ss'.000Z'");
    return QDateTime::fromString(str, fmt);
}

void StdInstVListLoadTask::processMCDLReply()
{
	switch (mcdlReply->error())
	{
	case QNetworkReply::NoError:
	{
		// Get the XML string.
		QString xmlString = mcdlReply->readAll();
		
		QString xmlErrorMsg;
		
		QDomDocument doc;
		if (!doc.setContent(xmlString, false, &xmlErrorMsg))
		{
			// TODO: Display error message to the user.
			qDebug(QString("Failed to process Minecraft download site. XML error: %s").
				   arg(xmlErrorMsg).toUtf8());
		}
		
		QDomNodeList contents = doc.elementsByTagName("Contents");
		
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
			QString dlUrl = "http://s3.amazonaws.com/MinecraftDownload/";
			
			if (key != "minecraft.jar")
				continue;
			
			QDateTime versionTimestamp = timeFromS3Time(lastModStr);
			if (!versionTimestamp.isValid())
			{
				qDebug(QString("Failed to parse timestamp for current stable version %1").
					   arg(lastModStr).toUtf8());
				versionTimestamp = QDateTime::currentDateTime();
			}
			
			currentStable = new StdInstVersion("LatestStable", "Current",
											   versionTimestamp.toMSecsSinceEpoch(),
											   "http://s3.amazonaws.com/MinecraftDownload/",
											   true, etagStr, m_list);
			
			setSubStatus("Loaded latest version info.");
		}
		break;
	}
		
	default:
		// TODO: Network error handling.
		break;
	}
	
	if (!currentStable)
		qDebug("Failed to get current stable version.");
	
	
	processedMCDLReply = true;
	updateStuff();
	
	// If the assets request isn't finished yet, connect the slot to allow it 
	// to process when the request is done. Otherwise, simply call the 
	// processAssetsReply slot directly.
	if (!assetsReply->isFinished())
		connect(assetsReply, SIGNAL(finished()),
				SLOT(processAssetsReply()));
	else if (!processedAssetsReply)
		processAssetsReply();
}

void StdInstVListLoadTask::processAssetsReply()
{
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
			qDebug(QString("Failed to process assets.minecraft.net. XML error: %s").
				   arg(xmlErrorMsg).toUtf8());
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
			
			if (currentStable)
			{
				if (etagStr == currentStable->etag())
				{
					StdInstVersion *version = new StdInstVersion(
								versionName, versionName,
								versionTimestamp.toMSecsSinceEpoch(),
								currentStable->downloadURL(), true, etagStr, m_list);
					version->setVersionType(StdInstVersion::CurrentStable);
					tempList.push_back(version);
					foundCurrentInAssets = true;
				}
				else
				{
					bool older = versionTimestamp.toMSecsSinceEpoch() < currentStable->timestamp();
					bool newer = versionTimestamp.toMSecsSinceEpoch() > currentStable->timestamp();
					bool isSnapshot = versionName.contains(snapshotRegex);
					
					StdInstVersion *version = new StdInstVersion(
								versionName, versionName, 
								versionTimestamp.toMSecsSinceEpoch(),
								dlUrl, false, etagStr, m_list);
					
					if (newer)
					{
						version->setVersionType(StdInstVersion::Snapshot);
					}
					else if (older && isSnapshot)
					{
						version->setVersionType(StdInstVersion::OldSnapshot);
					}
					else if (older)
					{
						version->setVersionType(StdInstVersion::Stable);
					}
					else
					{
						// Shouldn't happen, but just in case...
						version->setVersionType(StdInstVersion::CurrentStable);
					}
					
					tempList.push_back(version);
				}
			}
			else // If there isn't a current stable version.
			{
				bool isSnapshot = versionName.contains(snapshotRegex);
				
				StdInstVersion *version = new StdInstVersion(
							versionName, versionName, 
							versionTimestamp.toMSecsSinceEpoch(),
							dlUrl, false, etagStr, m_list);
				version->setVersionType(isSnapshot? StdInstVersion::Snapshot :
													StdInstVersion::Stable);
				tempList.push_back(version);
			}
		}
		
		setSubStatus("Loaded assets.minecraft.net");
		break;
	}
		
	default:
		// TODO: Network error handling.
		break;
	}
	
	processedAssetsReply = true;
	updateStuff();
}


QString mcnToAssetsVersion(QString mcnVersion);

void StdInstVListLoadTask::processMCNReply()
{
	switch (assetsReply->error())
	{
	case QNetworkReply::NoError:
	{
		QJsonParseError pError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(mcnReply->readAll(), &pError);
		
		if (pError.error != QJsonParseError::NoError)
		{
			// Handle errors.
			qDebug() << "Failed to parse MCNostalgia response. JSON parser error: " << 
						pError.errorString();
			break;
		}
		
		
		// Load data.
		QRegExp indevRegex("in(f)?dev");
		QJsonArray vlistArray = jsonDoc.object().value("order").toArray();
		
		for (int i = 0; i < vlistArray.size(); i++)
		{
			QString rawVersion = vlistArray.at(i).toString();
			if (rawVersion.isEmpty() || rawVersion.contains(indevRegex))
				continue;
			
			QString niceVersion = mcnToAssetsVersion(rawVersion);
			if (niceVersion.isEmpty())
				continue;
			
			StdInstVersion *version = StdInstVersion::mcnVersion(rawVersion, niceVersion);
			mcnList.prepend(version);
		}
		
		setSubStatus("Loaded MCNostalgia");
		break;
	}
		
	default:
		// TODO: Network error handling.
		break;
	}
	
	
	processedMCNReply = true;
	updateStuff();
}

void StdInstVListLoadTask::setSubStatus(const QString &msg)
{
	if (msg.isEmpty())
		setStatus("Loading instance version list...");
	else
		setStatus("Loading instance version list: " + msg);
}

void StdInstVListLoadTask::updateStuff()
{
	const int totalReqs = 3;
	int reqsComplete = 0;
	
	if (processedMCDLReply)
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

class MCNostalgiaVNameMap
{
public:
	QMap <QString, QString> mapping;
	MCNostalgiaVNameMap()
	{
		// An empty string means that it should be ignored
		mapping["1.4.6_pre"] = "";
		mapping["1.4.5_pre"] = "";
		mapping["1.4.3_pre"] = "1.4.3";
		mapping["1.4.2_pre"] = "";
		mapping["1.4.1_pre"] = "1.4.1";
		mapping["1.4_pre"] = "1.4";
		mapping["1.3.2_pre"] = "";
		mapping["1.3.1_pre"] = "";
		mapping["1.3_pre"] = "";
		mapping["1.2_pre"] = "1.2";
	}
} mcnVNMap;

QString mcnToAssetsVersion(QString mcnVersion)
{
	QMap<QString, QString>::iterator iter = mcnVNMap.mapping.find(mcnVersion);
	if (iter != mcnVNMap.mapping.end())
	{
		return iter.value();
	}
	return mcnVersion;
}
