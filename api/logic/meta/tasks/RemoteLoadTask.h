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

#pragma once

#include "tasks/Task.h"
#include <memory>

namespace Net
{
class Download;
}

namespace Meta
{
class BaseEntity;
class Index;
class VersionList;
class Version;

class RemoteLoadTask : public Task
{
	Q_OBJECT
public:
	explicit RemoteLoadTask(BaseEntity *entity, QObject *parent = nullptr);

protected:
	virtual QUrl url() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseEntity *entity() const { return m_entity; }

private slots:
	void networkFinished();

private:
	void executeTask() override;

	BaseEntity *m_entity;
	std::shared_ptr<Net::Download> m_dl;
};

class IndexRemoteLoadTask : public RemoteLoadTask
{
	Q_OBJECT
public:
	explicit IndexRemoteLoadTask(Index *index, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};

class VersionListRemoteLoadTask : public RemoteLoadTask
{
	Q_OBJECT
public:
	explicit VersionListRemoteLoadTask(VersionList *list, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	VersionList *list() const;
};

class VersionRemoteLoadTask : public RemoteLoadTask
{
	Q_OBJECT
public:
	explicit VersionRemoteLoadTask(Version *version, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	Version *version() const;
};
}
