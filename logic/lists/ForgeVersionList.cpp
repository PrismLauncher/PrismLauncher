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

#include <logger/QsLog.h>

#define JSON_URL "http://files.minecraftforge.net/minecraftforge/json"

ForgeVersionList::ForgeVersionList(QObject *parent) : BaseVersionList(parent)
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

int ForgeVersionList::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant ForgeVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<ForgeVersion>(m_vlist[index.row()]);
	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case 0:
			return version->name();

		case 1:
			return version->mcver;

		case 2:
			return version->typeString();
		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		return version->descriptor();

	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	default:
		return QVariant();
	}
}

QVariant ForgeVersionList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case 0:
			return "Version";

		case 1:
			return "Minecraft";

		case 2:
			return "Type";

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case 0:
			return "The name of the version.";

		case 1:
			return "Minecraft version";

		case 2:
			return "The version's type.";

		default:
			return QVariant();
		}

	default:
		return QVariant();
	}
}

BaseVersionPtr ForgeVersionList::getLatestStable() const
{
	return BaseVersionPtr();
}

void ForgeVersionList::updateListData(QList<BaseVersionPtr> versions)
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

ForgeListLoadTask::ForgeListLoadTask(ForgeVersionList *vlist) : Task()
{
	m_list = vlist;
}

void ForgeListLoadTask::executeTask()
{
	auto job = new DownloadJob("Version index");
	// we do not care if the version is stale or not.
	auto forgeListEntry = MMC->metacache()->resolveEntry("minecraftforge", "list.json");
	job->addCacheDownload(QUrl(JSON_URL), forgeListEntry);
	listJob.reset(job);
	connect(listJob.get(), SIGNAL(succeeded()), SLOT(list_downloaded()));
	connect(listJob.get(), SIGNAL(failed()), SLOT(list_failed()));
	connect(listJob.get(), SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	listJob->start();
}

void ForgeListLoadTask::list_failed()
{
	auto DlJob = listJob->first();
	auto reply = DlJob->m_reply;
	if(reply)
	{
		QLOG_ERROR() << "Getting forge version list failed: " << reply->errorString();
	}
	else
		QLOG_ERROR() << "Getting forge version list failed for reasons unknown.";
}

void ForgeListLoadTask::list_downloaded()
{
	QByteArray data;
	{
		auto DlJob = listJob->first();
		auto filename = std::dynamic_pointer_cast<CacheDownload>(DlJob)->m_target_path;
		QFile listFile(filename);
		if(!listFile.open(QIODevice::ReadOnly))
			return;
		data = listFile.readAll();
		DlJob.reset();
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

	QJsonObject root = jsonDoc.object();

	// Now, get the array of versions.
	if (!root.value("builds").isArray())
	{
		emitFailed(
			"Error parsing version list JSON: version list object is missing 'builds' array");
		return;
	}
	QJsonArray builds = root.value("builds").toArray();

	QList<BaseVersionPtr> tempList;
	for (int i = 0; i < builds.count(); i++)
	{
		// Load the version info.
		if (!builds[i].isObject())
		{
			// FIXME: log this somewhere
			continue;
		}
		QJsonObject obj = builds[i].toObject();
		int build_nr = obj.value("build").toDouble(0);
		if (!build_nr)
			continue;
		QJsonArray files = obj.value("files").toArray();
		QString url, jobbuildver, mcver, buildtype, filename;
		QString changelog_url, installer_url;
		QString installer_filename;
		bool valid = false;
		for (int j = 0; j < files.count(); j++)
		{
			if (!files[j].isObject())
				continue;
			QJsonObject file = files[j].toObject();
			buildtype = file.value("buildtype").toString();
			if ((buildtype == "client" || buildtype == "universal") && !valid)
			{
				mcver = file.value("mcver").toString();
				url = file.value("url").toString();
				jobbuildver = file.value("jobbuildver").toString();
				int lastSlash = url.lastIndexOf('/');
				filename = url.mid(lastSlash + 1);
				valid = true;
			}
			else if (buildtype == "changelog")
			{
				QString ext = file.value("ext").toString();
				if (ext.isEmpty())
					continue;
				changelog_url = file.value("url").toString();
			}
			else if (buildtype == "installer")
			{
				installer_url = file.value("url").toString();
				int lastSlash = installer_url.lastIndexOf('/');
				installer_filename = installer_url.mid(lastSlash + 1);
			}
		}
		if (valid)
		{
			// Now, we construct the version object and add it to the list.
			std::shared_ptr<ForgeVersion> fVersion(new ForgeVersion());
			fVersion->universal_url = url;
			fVersion->changelog_url = changelog_url;
			fVersion->installer_url = installer_url;
			fVersion->jobbuildver = jobbuildver;
			fVersion->mcver = mcver;
			if (installer_filename.isEmpty())
				fVersion->filename = filename;
			else
				fVersion->filename = installer_filename;
			fVersion->m_buildnr = build_nr;
			tempList.append(fVersion);
		}
	}
	m_list->updateListData(tempList);

	emitSucceeded();
	return;
}
