/* Copyright 2013-2017 MultiMC Contributors
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

#include <QObject>
#include <QAbstractListModel>

#include "BaseVersionList.h"
#include "tasks/Task.h"

#include "JavaCheckerJob.h"
#include "JavaInstall.h"

#include "multimc_logic_export.h"

class JavaListLoadTask;

class MULTIMC_LOGIC_EXPORT JavaInstallList : public BaseVersionList
{
	Q_OBJECT
	enum class Status
	{
		NotDone,
		InProgress,
		Done
	};
public:
	explicit JavaInstallList(QObject *parent = 0);

	shared_qobject_ptr<Task> getLoadTask() override;
	bool isLoaded() override;
	const BaseVersionPtr at(int i) const override;
	int count() const override;
	void sortVersions() override;

	QVariant data(const QModelIndex &index, int role) const override;
	RoleList providesRoles() const override;

public slots:
	void updateListData(QList<BaseVersionPtr> versions) override;

protected:
	void load();
	shared_qobject_ptr<Task> getCurrentTask();

protected:
	Status m_status = Status::NotDone;
	shared_qobject_ptr<JavaListLoadTask> m_loadTask;
	QList<BaseVersionPtr> m_vlist;
};

class JavaListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit JavaListLoadTask(JavaInstallList *vlist);
	~JavaListLoadTask();

	void executeTask() override;
public slots:
	void javaCheckerFinished(QList<JavaCheckResult> results);

protected:
	std::shared_ptr<JavaCheckerJob> m_job;
	JavaInstallList *m_list;
	JavaInstall *m_currentRecommended;
};
