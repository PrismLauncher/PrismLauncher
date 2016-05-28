/* Copyright 2015 MultiMC Contributors
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

#include "BaseWonkoEntityRemoteLoadTask.h"

#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "wonko/format/WonkoFormat.h"
#include "wonko/WonkoUtil.h"
#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersion.h"
#include "wonko/WonkoVersionList.h"
#include "Env.h"
#include "Json.h"

BaseWonkoEntityRemoteLoadTask::BaseWonkoEntityRemoteLoadTask(BaseWonkoEntity *entity, QObject *parent)
	: Task(parent), m_entity(entity)
{
}

void BaseWonkoEntityRemoteLoadTask::executeTask()
{
	NetJob *job = new NetJob(name());

	auto entry = ENV.metacache()->resolveEntry("wonko", url().toString());
	entry->setStale(true);
	m_dl = Net::Download::makeCached(url(), entry);
	job->addNetAction(m_dl);
	connect(job, &NetJob::failed, this, &BaseWonkoEntityRemoteLoadTask::emitFailed);
	connect(job, &NetJob::succeeded, this, &BaseWonkoEntityRemoteLoadTask::networkFinished);
	connect(job, &NetJob::status, this, &BaseWonkoEntityRemoteLoadTask::setStatus);
	connect(job, &NetJob::progress, this, &BaseWonkoEntityRemoteLoadTask::setProgress);
	job->start();
}

void BaseWonkoEntityRemoteLoadTask::networkFinished()
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

//      WONKO INDEX      //
WonkoIndexRemoteLoadTask::WonkoIndexRemoteLoadTask(WonkoIndex *index, QObject *parent)
	: BaseWonkoEntityRemoteLoadTask(index, parent)
{
}
QUrl WonkoIndexRemoteLoadTask::url() const
{
	return Wonko::indexUrl();
}
QString WonkoIndexRemoteLoadTask::name() const
{
	return tr("Wonko Index");
}
void WonkoIndexRemoteLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseIndex(obj, dynamic_cast<WonkoIndex *>(entity()));
}

//      WONKO VERSION LIST      //
WonkoVersionListRemoteLoadTask::WonkoVersionListRemoteLoadTask(WonkoVersionList *list, QObject *parent)
	: BaseWonkoEntityRemoteLoadTask(list, parent)
{
}
QUrl WonkoVersionListRemoteLoadTask::url() const
{
	return Wonko::versionListUrl(list()->uid());
}
QString WonkoVersionListRemoteLoadTask::name() const
{
	return tr("Wonko Version List for %1").arg(list()->humanReadable());
}
void WonkoVersionListRemoteLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseVersionList(obj, list());
}
WonkoVersionList *WonkoVersionListRemoteLoadTask::list() const
{
	return dynamic_cast<WonkoVersionList *>(entity());
}

//      WONKO VERSION      //
WonkoVersionRemoteLoadTask::WonkoVersionRemoteLoadTask(WonkoVersion *version, QObject *parent)
	: BaseWonkoEntityRemoteLoadTask(version, parent)
{
}
QUrl WonkoVersionRemoteLoadTask::url() const
{
	return Wonko::versionUrl(version()->uid(), version()->version());
}
QString WonkoVersionRemoteLoadTask::name() const
{
	return tr("Wonko Version for %1").arg(version()->name());
}
void WonkoVersionRemoteLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseVersion(obj, version());
}
WonkoVersion *WonkoVersionRemoteLoadTask::version() const
{
	return dynamic_cast<WonkoVersion *>(entity());
}
