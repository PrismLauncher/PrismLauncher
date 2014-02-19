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

#include "LiteLoaderVersionList.h"
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

LiteLoaderVersionList::LiteLoaderVersionList(QObject *parent) : BaseVersionList(parent)
{
}

Task *LiteLoaderVersionList::getLoadTask()
{
	return new LLListLoadTask(this);
}

bool LiteLoaderVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr LiteLoaderVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int LiteLoaderVersionList::count() const
{
	return m_vlist.count();
}

static bool cmpVersions(BaseVersionPtr first, BaseVersionPtr second)
{
	auto left = std::dynamic_pointer_cast<LiteLoaderVersion>(first);
	auto right = std::dynamic_pointer_cast<LiteLoaderVersion>(second);
	return left->timestamp > right->timestamp;
}

void LiteLoaderVersionList::sort()
{
	beginResetModel();
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}

BaseVersionPtr LiteLoaderVersionList::getLatestStable() const
{
	for (int i = 0; i < m_vlist.length(); i++)
	{
		auto ver = std::dynamic_pointer_cast<LiteLoaderVersion>(m_vlist.at(i));
		if (ver->isLatest)
		{
			return m_vlist.at(i);
		}
	}
	return BaseVersionPtr();
}

void LiteLoaderVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
	endResetModel();
}

LLListLoadTask::LLListLoadTask(LiteLoaderVersionList *vlist)
{
	m_list = vlist;
	vlistReply = nullptr;
}

LLListLoadTask::~LLListLoadTask()
{
}

void LLListLoadTask::executeTask()
{
	setStatus(tr("Loading LiteLoader version list..."));
	auto worker = MMC->qnam();
	vlistReply = worker->get(QNetworkRequest(QUrl(URLConstants::LITELOADER_URL)));
	connect(vlistReply, SIGNAL(finished()), this, SLOT(listDownloaded()));
}

void LLListLoadTask::listDownloaded()
{
	if (vlistReply->error() != QNetworkReply::NoError)
	{
		vlistReply->deleteLater();
		emitFailed("Failed to load LiteLoader version list" + vlistReply->errorString());
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

	const QJsonObject root = jsonDoc.object();

	// Now, get the array of versions.
	if (!root.value("versions").isObject())
	{
		emitFailed("Error parsing version list JSON: missing 'versions' object");
		return;
	}
	const QJsonObject versions = root.value("versions").toObject();

	QList<BaseVersionPtr> tempList;
	for (auto vIt = versions.begin(); vIt != versions.end(); ++vIt)
	{
		const QString mcVersion = vIt.key();
		QString latest;
		const QJsonObject artefacts = vIt.value()
										  .toObject()
										  .value("artefacts")
										  .toObject()
										  .value("com.mumfrey:liteloader")
										  .toObject();
		QList<BaseVersionPtr> perMcVersionList;
		for (auto aIt = artefacts.begin(); aIt != artefacts.end(); ++aIt)
		{
			const QString identifier = aIt.key();
			const QJsonObject artefact = aIt.value().toObject();
			if (identifier == "latest")
			{
				latest = artefact.value("version").toString();
				continue;
			}
			LiteLoaderVersionPtr version(new LiteLoaderVersion());
			version->version = artefact.value("version").toString();
			version->file = artefact.value("file").toString();
			version->mcVersion = mcVersion;
			version->md5 = artefact.value("md5").toString();
			version->timestamp = artefact.value("timestamp").toDouble();
			version->tweakClass = artefact.value("tweakClass").toString();
			const QJsonArray libs = artefact.value("libraries").toArray();
			for (auto lIt = libs.begin(); lIt != libs.end(); ++lIt)
			{
				version->libraries.append((*lIt).toObject().value("name").toString());
			}
			perMcVersionList.append(version);
		}
		for (auto version : perMcVersionList)
		{
			auto v = std::dynamic_pointer_cast<LiteLoaderVersion>(version);
			v->isLatest = v->version == latest;
		}
		tempList.append(perMcVersionList);
	}
	m_list->updateListData(tempList);

	emitSucceeded();
}
