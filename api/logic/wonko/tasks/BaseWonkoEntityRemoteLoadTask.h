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

#pragma once

#include "tasks/Task.h"
#include <memory>

class BaseWonkoEntity;
class WonkoIndex;
class WonkoVersionList;
class WonkoVersion;

class BaseWonkoEntityRemoteLoadTask : public Task
{
	Q_OBJECT
public:
	explicit BaseWonkoEntityRemoteLoadTask(BaseWonkoEntity *entity, QObject *parent = nullptr);

protected:
	virtual QUrl url() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseWonkoEntity *entity() const { return m_entity; }

private slots:
	void networkFinished();

private:
	void executeTask() override;

	BaseWonkoEntity *m_entity;
	std::shared_ptr<class CacheDownload> m_dl;
};

class WonkoIndexRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoIndexRemoteLoadTask(WonkoIndex *index, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};
class WonkoVersionListRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionListRemoteLoadTask(WonkoVersionList *list, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersionList *list() const;
};
class WonkoVersionRemoteLoadTask : public BaseWonkoEntityRemoteLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionRemoteLoadTask(WonkoVersion *version, QObject *parent = nullptr);

private:
	QUrl url() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersion *version() const;
};
