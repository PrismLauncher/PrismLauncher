/* Copyright 2015-2017 MultiMC Contributors
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

#include "RemoteLoadTask.h"

#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "meta/format/Format.h"
#include "meta/Util.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "Env.h"
#include "Json.h"

namespace Meta
{

RemoteLoadTask::RemoteLoadTask(BaseEntity *entity, QObject *parent)
	: Task(parent), m_entity(entity)
{
}

void RemoteLoadTask::executeTask()
{
	NetJob *job = new NetJob(name());

	auto entry = ENV.metacache()->resolveEntry("meta", url().toString());
	entry->setStale(true);
	m_dl = Net::Download::makeCached(url(), entry);
	job->addNetAction(m_dl);
	connect(job, &NetJob::failed, this, &RemoteLoadTask::emitFailed);
	connect(job, &NetJob::succeeded, this, &RemoteLoadTask::networkFinished);
	connect(job, &NetJob::status, this, &RemoteLoadTask::setStatus);
	connect(job, &NetJob::progress, this, &RemoteLoadTask::setProgress);
	job->start();
}

void RemoteLoadTask::networkFinished()
{
	setStatus(tr("Parsing..."));
	setProgress(0, 0);

	try
	{
		parse(Json::requireObject(Json::requireDocument(m_dl->getTargetFilepath(), name()), name()));
		m_entity->notifyRemoteLoadComplete();
		emitSucceeded();
	}
	catch (Exception &e)
	{
		emitFailed(tr("Unable to parse response: %1").arg(e.cause()));
	}
}

// INDEX
IndexRemoteLoadTask::IndexRemoteLoadTask(Index *index, QObject *parent)
	: RemoteLoadTask(index, parent)
{
}
QUrl IndexRemoteLoadTask::url() const
{
	return Meta::indexUrl();
}
QString IndexRemoteLoadTask::name() const
{
	return tr("Metadata Index");
}
void IndexRemoteLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseIndex(obj, dynamic_cast<Index *>(entity()));
}


// VERSION LIST
VersionListRemoteLoadTask::VersionListRemoteLoadTask(VersionList *list, QObject *parent)
	: RemoteLoadTask(list, parent)
{
}
QUrl VersionListRemoteLoadTask::url() const
{
	return Meta::versionListUrl(list()->uid());
}
QString VersionListRemoteLoadTask::name() const
{
	return tr("Version List for %1").arg(list()->humanReadable());
}
void VersionListRemoteLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseVersionList(obj, list());
}
VersionList* Meta::VersionListRemoteLoadTask::list() const
{
	return dynamic_cast<VersionList *>(entity());
}


// VERSION
VersionRemoteLoadTask::VersionRemoteLoadTask(Version *version, QObject *parent)
	: RemoteLoadTask(version, parent)
{
}
QUrl VersionRemoteLoadTask::url() const
{
	return Meta::versionUrl(version()->uid(), version()->version());
}
QString VersionRemoteLoadTask::name() const
{
	return tr("Meta Version for %1").arg(version()->name());
}
void VersionRemoteLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseVersion(obj, version());
}
Version *VersionRemoteLoadTask::version() const
{
	return dynamic_cast<Version *>(entity());
}
}
