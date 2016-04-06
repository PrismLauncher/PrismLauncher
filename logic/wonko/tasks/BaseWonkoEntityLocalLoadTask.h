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

class BaseWonkoEntityLocalLoadTask : public Task
{
	Q_OBJECT
public:
	explicit BaseWonkoEntityLocalLoadTask(BaseWonkoEntity *entity, QObject *parent = nullptr);

protected:
	virtual QString filename() const = 0;
	virtual QString name() const = 0;
	virtual void parse(const QJsonObject &obj) const = 0;

	BaseWonkoEntity *entity() const { return m_entity; }

private:
	void executeTask() override;

	BaseWonkoEntity *m_entity;
};

class WonkoIndexLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoIndexLocalLoadTask(WonkoIndex *index, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;
};
class WonkoVersionListLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionListLocalLoadTask(WonkoVersionList *list, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersionList *list() const;
};
class WonkoVersionLocalLoadTask : public BaseWonkoEntityLocalLoadTask
{
	Q_OBJECT
public:
	explicit WonkoVersionLocalLoadTask(WonkoVersion *version, QObject *parent = nullptr);

private:
	QString filename() const override;
	QString name() const override;
	void parse(const QJsonObject &obj) const override;

	WonkoVersion *version() const;
};
