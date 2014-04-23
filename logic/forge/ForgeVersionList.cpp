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

#include "logic/forge/ForgeVersionList.h"
#include "logic/forge/ForgeVersion.h"
#include "logic/net/NetJob.h"
#include "logic/net/URLConstants.h"
#include "MultiMC.h"

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include "logger/QsLog.h"

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
	setStatus(tr("Fetching Forge version lists..."));
	auto job = new NetJob("Version index");
	// we do not care if the version is stale or not.
	auto forgeListEntry = MMC->metacache()->resolveEntry("minecraftforge", "list.json");
	auto gradleForgeListEntry = MMC->metacache()->resolveEntry("minecraftforge", "json");

	// verify by poking the server.
	forgeListEntry->stale = true;
	gradleForgeListEntry->stale = true;

	job->addNetAction(listDownload = CacheDownload::make(QUrl(URLConstants::FORGE_LEGACY_URL),
														 forgeListEntry));
	job->addNetAction(gradleListDownload = CacheDownload::make(
						  QUrl(URLConstants::FORGE_GRADLE_URL), gradleForgeListEntry));

	connect(listDownload.get(), SIGNAL(failed(int)), SLOT(listFailed()));
	connect(gradleListDownload.get(), SIGNAL(failed(int)), SLOT(gradleListFailed()));

	listJob.reset(job);
	connect(listJob.get(), SIGNAL(succeeded()), SLOT(listDownloaded()));
	connect(listJob.get(), SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	listJob->start();
}

bool ForgeListLoadTask::parseForgeList(QList<BaseVersionPtr> &out)
{
	QByteArray data;
	{
		auto dlJob = listDownload;
		auto filename = std::dynamic_pointer_cast<CacheDownload>(dlJob)->getTargetFilepath();
		QFile listFile(filename);
		if (!listFile.open(QIODevice::ReadOnly))
		{
			return false;
		}
		data = listFile.readAll();
		dlJob.reset();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed("Error parsing version list JSON:" + jsonError.errorString());
		return false;
	}

	if (!jsonDoc.isObject())
	{
		emitFailed("Error parsing version list JSON: JSON root is not an object");
		return false;
	}

	QJsonObject root = jsonDoc.object();

	// Now, get the array of versions.
	if (!root.value("builds").isArray())
	{
		emitFailed(
			"Error parsing version list JSON: version list object is missing 'builds' array");
		return false;
	}
	QJsonArray builds = root.value("builds").toArray();

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
		QString url, jobbuildver, mcver, buildtype, universal_filename;
		QString changelog_url, installer_url;
		QString installer_filename;
		bool valid = false;
		for (int j = 0; j < files.count(); j++)
		{
			if (!files[j].isObject())
			{
				continue;
			}
			QJsonObject file = files[j].toObject();
			buildtype = file.value("buildtype").toString();
			if ((buildtype == "client" || buildtype == "universal") && !valid)
			{
				mcver = file.value("mcver").toString();
				url = file.value("url").toString();
				jobbuildver = file.value("jobbuildver").toString();
				int lastSlash = url.lastIndexOf('/');
				universal_filename = url.mid(lastSlash + 1);
				valid = true;
			}
			else if (buildtype == "changelog")
			{
				QString ext = file.value("ext").toString();
				if (ext.isEmpty())
				{
					continue;
				}
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
			fVersion->installer_filename = installer_filename;
			fVersion->universal_filename = universal_filename;
			fVersion->m_buildnr = build_nr;
			out.append(fVersion);
		}
	}

	return true;
}

bool ForgeListLoadTask::parseForgeGradleList(QList<BaseVersionPtr> &out)
{
	QByteArray data;
	{
		auto dlJob = gradleListDownload;
		auto filename = std::dynamic_pointer_cast<CacheDownload>(dlJob)->getTargetFilepath();
		QFile listFile(filename);
		if (!listFile.open(QIODevice::ReadOnly))
		{
			return false;
		}
		data = listFile.readAll();
		dlJob.reset();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed("Error parsing gradle version list JSON:" + jsonError.errorString());
		return false;
	}

	if (!jsonDoc.isObject())
	{
		emitFailed("Error parsing gradle version list JSON: JSON root is not an object");
		return false;
	}

	QJsonObject root = jsonDoc.object();

	// we probably could hard code these, but it might still be worth doing it this way
	const QString webpath = root.value("webpath").toString();
	const QString artifact = root.value("artifact").toString();

	QJsonObject numbers = root.value("number").toObject();
	for (auto it = numbers.begin(); it != numbers.end(); ++it)
	{
		QJsonObject number = it.value().toObject();
		std::shared_ptr<ForgeVersion> fVersion(new ForgeVersion());
		fVersion->m_buildnr = number.value("build").toDouble();
		fVersion->jobbuildver = number.value("version").toString();
		fVersion->mcver = number.value("mcversion").toString();
		fVersion->universal_filename = "";
		QString filename, installer_filename;
		QJsonArray files = number.value("files").toArray();
		for (auto fIt = files.begin(); fIt != files.end(); ++fIt)
		{
			// TODO with gradle we also get checksums, use them
			QJsonArray file = (*fIt).toArray();
			if (file.size() < 3)
			{
				continue;
			}
			if (file.at(1).toString() == "installer")
			{
				fVersion->installer_url = QString("%1/%2-%3/%4-%2-%3-installer.%5").arg(
					webpath, fVersion->mcver, fVersion->jobbuildver, artifact,
					file.at(0).toString());
				installer_filename = QString("%1-%2-%3-installer.%4").arg(
					artifact, fVersion->mcver, fVersion->jobbuildver, file.at(0).toString());
			}
			else if (file.at(1).toString() == "universal")
			{
				fVersion->universal_url = QString("%1/%2-%3/%4-%2-%3-universal.%5").arg(
					webpath, fVersion->mcver, fVersion->jobbuildver, artifact,
					file.at(0).toString());
				filename = QString("%1-%2-%3-universal.%4").arg(
					artifact, fVersion->mcver, fVersion->jobbuildver, file.at(0).toString());
			}
			else if (file.at(1).toString() == "changelog")
			{
				fVersion->changelog_url = QString("%1/%2-%3/%4-%2-%3-changelog.%5").arg(
					webpath, fVersion->mcver, fVersion->jobbuildver, artifact,
					file.at(0).toString());
			}
		}
		if (fVersion->installer_url.isEmpty() && fVersion->universal_url.isEmpty())
		{
			continue;
		}
		fVersion->universal_filename = filename;
		fVersion->installer_filename = installer_filename;
		out.append(fVersion);
	}

	return true;
}

void ForgeListLoadTask::listDownloaded()
{
	QList<BaseVersionPtr> list;
	bool ret = true;
	if (!parseForgeList(list))
	{
		ret = false;
	}
	if (!parseForgeGradleList(list))
	{
		ret = false;
	}

	if (!ret)
	{
		return;
	}
	std::sort(list.begin(), list.end(), [](const BaseVersionPtr & l, const BaseVersionPtr & r)
	{ return (*l > *r); });

	m_list->updateListData(list);

	emitSucceeded();
	return;
}

void ForgeListLoadTask::listFailed()
{
	auto reply = listDownload->m_reply;
	if (reply)
	{
		QLOG_ERROR() << "Getting forge version list failed: " << reply->errorString();
	}
	else
	{
		QLOG_ERROR() << "Getting forge version list failed for reasons unknown.";
	}
}
void ForgeListLoadTask::gradleListFailed()
{
	auto reply = gradleListDownload->m_reply;
	if (reply)
	{
		QLOG_ERROR() << "Getting forge version list failed: " << reply->errorString();
	}
	else
	{
		QLOG_ERROR() << "Getting forge version list failed for reasons unknown.";
	}
}
