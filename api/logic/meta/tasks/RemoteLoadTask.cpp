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

class ParsingValidator : public Net::Validator
{
public: /* con/des */
	ParsingValidator(BaseEntity *entity) : m_entity(entity)
	{
	};
	virtual ~ParsingValidator()
	{
	};

public: /* methods */
	bool init(QNetworkRequest &) override
	{
		return true;
	}
	bool write(QByteArray & data) override
	{
		this->data.append(data);
		return true;
	}
	bool abort() override
	{
		return true;
	}
	bool validate(QNetworkReply &) override
	{
		auto fname = m_entity->localFilename();
		try
		{
			m_entity->parse(Json::requireObject(Json::requireDocument(data, fname), fname));
			m_entity->notifyRemoteLoadComplete();
			return true;
		}
		catch (Exception &e)
		{
			qWarning() << "Unable to parse response:" << e.cause();
			return false;
		}
	}

private: /* data */
	QByteArray data;
	BaseEntity *m_entity;
};

void RemoteLoadTask::executeTask()
{
	// FIXME: leak here!!!
	NetJob *job = new NetJob(tr("Download of meta file %1").arg(m_entity->localFilename()));

	auto url = m_entity->url();
	auto entry = ENV.metacache()->resolveEntry("meta", m_entity->localFilename());
	entry->setStale(true);
	m_dl = Net::Download::makeCached(url, entry);
	/*
	 * The validator parses the file and loads it into the object.
	 * If that fails, the file is not written to storage.
	 */
	m_dl->addValidator(new ParsingValidator(m_entity));
	job->addNetAction(m_dl);
	connect(job, &NetJob::failed, this, &RemoteLoadTask::emitFailed);
	connect(job, &NetJob::succeeded, this, &RemoteLoadTask::succeeded);
	connect(job, &NetJob::status, this, &RemoteLoadTask::setStatus);
	connect(job, &NetJob::progress, this, &RemoteLoadTask::setProgress);
	job->start();
}
}
