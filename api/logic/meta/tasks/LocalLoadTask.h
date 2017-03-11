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

namespace Meta
{
class BaseEntity;
class Index;
class VersionList;
class Version;

class LocalLoadTask : public Task
{
	Q_OBJECT
public:
	explicit LocalLoadTask(BaseEntity *entity, QObject *parent = nullptr);

protected:
	virtual QString filename() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseEntity *entity() const { return m_entity; }

private:
	void executeTask() override;

	BaseEntity *m_entity;
};

class IndexLocalLoadTask : public LocalLoadTask
{
	Q_OBJECT
public:
	explicit IndexLocalLoadTask(Index *index, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};
class VersionListLocalLoadTask : public LocalLoadTask
{
	Q_OBJECT
public:
	explicit VersionListLocalLoadTask(VersionList *list, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	VersionList *list() const;
};
class VersionLocalLoadTask : public LocalLoadTask
{
	Q_OBJECT
public:
	explicit VersionLocalLoadTask(Version *version, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	Version *version() const;
};
}
