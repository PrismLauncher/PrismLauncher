/* Copyright 2015-2018 MultiMC Contributors
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

#include "BaseEntity.h"

#include "Json.h"

#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"

#include "Env.h"
#include "Json.h"

class ParsingValidator : public Net::Validator
{
public: /* con/des */
	ParsingValidator(Meta::BaseEntity *entity) : m_entity(entity)
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
	Meta::BaseEntity *m_entity;
};

Meta::BaseEntity::~BaseEntity()
{
}

QUrl Meta::BaseEntity::url() const
{
	return QUrl("https://v1.meta.multimc.org").resolved(localFilename());
}

bool Meta::BaseEntity::loadLocalFile()
{
	const QString fname = QDir("meta").absoluteFilePath(localFilename());
	if (!QFile::exists(fname))
	{
		return false;
	}
	// TODO: check if the file has the expected checksum
	try
	{
		parse(Json::requireObject(Json::requireDocument(fname, fname), fname));
		return true;
	}
	catch (Exception &e)
	{
		qDebug() << QString("Unable to parse file %1: %2").arg(fname, e.cause());
		// just make sure it's gone and we never consider it again.
		QFile::remove(fname);
		return false;
	}
}

void Meta::BaseEntity::load(Net::Mode loadType)
{
	// load local file if nothing is loaded yet
	if(!isLoaded())
	{
		if(loadLocalFile())
		{
			m_loadStatus = LoadStatus::Local;
		}
	}
	// if we need remote update, run the update task
	if(loadType == Net::Mode::Offline || !shouldStartRemoteUpdate())
	{
		return;
	}
	NetJob *job = new NetJob(QObject::tr("Download of meta file %1").arg(localFilename()));
	auto url = this->url();
	auto entry = ENV.metacache()->resolveEntry("meta", localFilename());
	entry->setStale(true);
	auto dl = Net::Download::makeCached(url, entry);
	/*
	 * The validator parses the file and loads it into the object.
	 * If that fails, the file is not written to storage.
	 */
	dl->addValidator(new ParsingValidator(this));
	job->addNetAction(dl);
	m_updateStatus = UpdateStatus::InProgress;
	m_updateTask.reset(job);
	QObject::connect(job, &NetJob::succeeded, [&]()
	{
		m_loadStatus = LoadStatus::Remote;
		m_updateStatus = UpdateStatus::Succeeded;
		m_updateTask.reset();
	});
	QObject::connect(job, &NetJob::failed, [&]()
	{
		m_updateStatus = UpdateStatus::Failed;
		m_updateTask.reset();
	});
	m_updateTask->start();
}

bool Meta::BaseEntity::isLoaded() const
{
	return m_loadStatus > LoadStatus::NotLoaded;
}

bool Meta::BaseEntity::shouldStartRemoteUpdate() const
{
	// TODO: version-locks and offline mode?
	return m_updateStatus != UpdateStatus::InProgress;
}

shared_qobject_ptr<Task> Meta::BaseEntity::getCurrentTask()
{
	if(m_updateStatus == UpdateStatus::InProgress)
	{
		return m_updateTask;
	}
	return nullptr;
}
