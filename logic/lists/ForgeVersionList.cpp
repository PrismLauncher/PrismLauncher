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

#include "ForgeVersionList.h"
#include <logic/net/DownloadJob.h>
#include "MultiMC.h"

#include <QtNetwork>

#include <QtXml>

#include <QRegExp>

#define JSON_URL "http://files.minecraftforge.net/minecraftforge/json"


ForgeVersionList::ForgeVersionList(QObject* parent): BaseVersionList(parent)
{
	
}

Task *ForgeVersionList::getLoadTask()
{
	return new ForgeListLoadTask(this);
}

bool ForgeVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr ForgeVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int ForgeVersionList::count() const
{
	return m_vlist.count();
}
/*
bool cmpVersions(BaseVersionPtr first, BaseVersionPtr second)
{
	const BaseVersion & left = *first;
	const BaseVersion & right = *second;
	return left > right;
}

void MinecraftVersionList::sort()
{
	beginResetModel();
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}
*/
BaseVersionPtr ForgeVersionList::getLatestStable() const
{
	return BaseVersionPtr();
}

void ForgeVersionList::updateListData(QList<BaseVersionPtr > versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	endResetModel();
	// NOW SORT!!
	// sort();
}

void ForgeVersionList::sort()
{
	// NO-OP for now
}


ForgeListLoadTask::ForgeListLoadTask(ForgeVersionList* vlist): Task()
{
	m_list = vlist;
}


void ForgeListLoadTask::executeTask()
{
	auto job = new DownloadJob("Version index");
	job->add(QUrl(JSON_URL));
	listJob.reset(job);
	connect(listJob.data(), SIGNAL(succeeded()), SLOT(list_downloaded()));
	connect(listJob.data(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(listJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	listJob->start();
}

void ForgeListLoadTask::list_downloaded()
{
	auto DlJob = listJob->first();
	auto data = DlJob.dynamicCast<ByteArrayDownload>()->m_data;
	
	
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	DlJob.reset();
	
	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed("Error parsing version list JSON:" + jsonError.errorString());
		return;
	}

	if(!jsonDoc.isObject())
	{
		emitFailed("Error parsing version list JSON: jsonDoc is not an object");
		return;
	}
	
	QJsonObject root = jsonDoc.object();
	
	// Now, get the array of versions.
	if(!root.value("builds").isArray())
	{
		emitFailed("Error parsing version list JSON: version list object is missing 'builds' array");
		return;
	}
	QJsonArray builds = root.value("builds").toArray();
	
	QList<BaseVersionPtr > tempList;
	for (int i = 0; i < builds.count(); i++)
	{
		// Load the version info.
		if(!builds[i].isObject())
		{
			//FIXME: log this somewhere
			continue;
		}
		QJsonObject obj = builds[i].toObject();
		int build_nr = obj.value("build").toDouble(0);
		if(!build_nr)
			continue;
		QJsonArray files = root.value("files").toArray();
		QString url, jobbuildver, mcver, buildtype, filename;
		QString changelog_url, installer_url;
		bool valid = false;
		for(int j = 0; j < files.count(); j++)
		{
			if(!files[j].isObject())
				continue;
			QJsonObject file = files[j].toObject();
			buildtype = file.value("buildtype").toString();
			if((buildtype == "client" || buildtype == "universal") && !valid)
			{
				mcver = file.value("mcver").toString();
				url = file.value("url").toString();
				jobbuildver = file.value("jobbuildver").toString();
				int lastSlash = url.lastIndexOf('/');
				filename = url.mid(lastSlash+1);
				valid = true;
			}
			else if(buildtype == "changelog")
			{
				QString ext = file.value("ext").toString();
				if(ext.isEmpty())
					continue;
				changelog_url = file.value("url").toString();
			}
			else if(buildtype == "installer")
			{
				installer_url = file.value("url").toString();
			}
		}
		if(valid)
		{
			// Now, we construct the version object and add it to the list.
			QSharedPointer<ForgeVersion> fVersion(new ForgeVersion());
			fVersion->universal_url = url;
			fVersion->changelog_url = changelog_url;
			fVersion->installer_url = installer_url;
			fVersion->jobbuildver = jobbuildver;
			fVersion->mcver = mcver;
			fVersion->filename = filename;
			fVersion->m_buildnr = build_nr;
			tempList.append(fVersion);
		}
	}
	m_list->updateListData(tempList);
	
	emitSucceeded();
	return;
}
