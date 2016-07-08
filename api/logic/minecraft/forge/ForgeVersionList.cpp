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

#include "ForgeVersionList.h"
#include "ForgeVersion.h"

#include "net/NetJob.h"
#include "net/URLConstants.h"
#include "Env.h"

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include <QDebug>

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
	return 1;
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
	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	case VersionRole:
		return version->name();

	case VersionIdRole:
		return version->descriptor();

	case ParentGameVersionRole:
		return version->mcver_sane;

	case RecommendedRole:
		return version->is_recommended;

	case BranchRole:
		return version->branch;

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList ForgeVersionList::providesRoles() const
{
	return {VersionPointerRole, VersionRole, VersionIdRole, ParentGameVersionRole, RecommendedRole, BranchRole};
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

void ForgeVersionList::sortVersions()
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
	auto forgeListEntry = ENV.metacache()->resolveEntry("minecraftforge", "list.json");
	auto gradleForgeListEntry = ENV.metacache()->resolveEntry("minecraftforge", "json");

	// verify by poking the server.
	forgeListEntry->setStale(true);
	gradleForgeListEntry->setStale(true);

	job->addNetAction(listDownload = Net::Download::makeCached(QUrl(URLConstants::FORGE_LEGACY_URL),forgeListEntry));
	job->addNetAction(gradleListDownload = Net::Download::makeCached(QUrl(URLConstants::FORGE_GRADLE_URL), gradleForgeListEntry));

	connect(listDownload.get(), SIGNAL(failed(int)), SLOT(listFailed()));
	connect(gradleListDownload.get(), SIGNAL(failed(int)), SLOT(gradleListFailed()));

	listJob.reset(job);
	connect(listJob.get(), SIGNAL(succeeded()), SLOT(listDownloaded()));
	connect(listJob.get(), SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	listJob->start();
}

bool ForgeListLoadTask::abort()
{
	return listJob->abort();
}

bool ForgeListLoadTask::parseForgeGradleList(QList<BaseVersionPtr> &out)
{
	QMap<int, std::shared_ptr<ForgeVersion>> lookup;
	QByteArray data;
	{
		auto filename = gradleListDownload->getTargetFilepath();
		QFile listFile(filename);
		if (!listFile.open(QIODevice::ReadOnly))
		{
			return false;
		}
		data = listFile.readAll();
		gradleListDownload.reset();
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
		if(fVersion->m_buildnr >= 953 && fVersion->m_buildnr <= 965)
		{
			qDebug() << fVersion->m_buildnr;
		}
		fVersion->jobbuildver = number.value("version").toString();
		fVersion->branch = number.value("branch").toString("");
		fVersion->mcver = number.value("mcversion").toString();
		fVersion->universal_filename = "";
		fVersion->installer_filename = "";
		// HACK: here, we fix the minecraft version used by forge.
		// HACK: this will inevitably break (later)
		// FIXME: replace with a dictionary
		fVersion->mcver_sane = fVersion->mcver;
		fVersion->mcver_sane.replace("_pre", "-pre");

		QString universal_filename, installer_filename;
		QJsonArray files = number.value("files").toArray();
		for (auto fIt = files.begin(); fIt != files.end(); ++fIt)
		{
			// TODO with gradle we also get checksums, use them
			QJsonArray file = (*fIt).toArray();
			if (file.size() < 3)
			{
				continue;
			}

			QString extension = file.at(0).toString();
			QString part = file.at(1).toString();
			QString checksum = file.at(2).toString();

			// insane form of mcver is used here
			QString longVersion = fVersion->mcver + "-" + fVersion->jobbuildver;
			if (!fVersion->branch.isEmpty())
			{
				longVersion = longVersion + "-" + fVersion->branch;
			}
			QString filename = artifact + "-" + longVersion + "-" + part + "." + extension;

			QString url = QString("%1/%2/%3")
							  .arg(webpath)
							  .arg(longVersion)
							  .arg(filename);

			if (part == "installer")
			{
				fVersion->installer_url = url;
				installer_filename = filename;
			}
			else if (part == "universal" || part == "client")
			{
				fVersion->universal_url = url;
				universal_filename = filename;
			}
			else if (part == "changelog")
			{
				fVersion->changelog_url = url;
			}
		}
		if (fVersion->installer_url.isEmpty() && fVersion->universal_url.isEmpty())
		{
			continue;
		}
		fVersion->universal_filename = universal_filename;
		fVersion->installer_filename = installer_filename;
		fVersion->type = ForgeVersion::Gradle;
		out.append(fVersion);
		lookup[fVersion->m_buildnr] = fVersion;
	}
	QJsonObject promos = root.value("promos").toObject();
	for (auto it = promos.begin(); it != promos.end(); ++it)
	{
		QString key = it.key();
		int build = it.value().toInt();
		QRegularExpression regexp("^(?<mcversion>[0-9]+(.[0-9]+)*)-(?<label>[a-z]+)$");
		auto match = regexp.match(key);
		if(!match.hasMatch())
		{
			qDebug() << key << "doesn't match." << "build" << build;
			continue;
		}

		QString label = match.captured("label");
		if(label != "recommended")
		{
			continue;
		}
		QString mcversion = match.captured("mcversion");
		qDebug() << "Forge build" << build << "is the" << label << "for Minecraft" << mcversion << QString("<%1>").arg(key);
		lookup[build]->is_recommended = true;
	}
	return true;
}

void ForgeListLoadTask::listDownloaded()
{
	QList<BaseVersionPtr> list;
	bool ret = true;

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
	auto &reply = listDownload->m_reply;
	if (reply)
	{
		qCritical() << "Getting forge version list failed: " << reply->errorString();
	}
	else
	{
		qCritical() << "Getting forge version list failed for reasons unknown.";
	}
}

void ForgeListLoadTask::gradleListFailed()
{
	auto &reply = gradleListDownload->m_reply;
	if (reply)
	{
		qCritical() << "Getting forge version list failed: " << reply->errorString();
	}
	else
	{
		qCritical() << "Getting forge version list failed for reasons unknown.";
	}
}
