/* Copyright 2013-2021 MultiMC Contributors
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

#include <QAbstractListModel>
#include <QObject>

#include "BaseVersionList.h"
#include "tasks/Task.h"

#include "JavaCheckerJob.h"
#include "JavaInstall.h"

#include "QObjectPtr.h"

class JavaListLoadTask;

class JavaInstallList : public BaseVersionList {
    Q_OBJECT
    enum class Status { NotDone, InProgress, Done };

   public:
    explicit JavaInstallList(QObject* parent = 0);

    Task::Ptr getLoadTask() override;
    bool isLoaded() override;
    const BaseVersion::Ptr at(int i) const override;
    int count() const override;
    void sortVersions() override;

    QVariant data(const QModelIndex& index, int role) const override;
    RoleList providesRoles() const override;

   public slots:
    void updateListData(QList<BaseVersion::Ptr> versions) override;

   protected:
    void load();
    Task::Ptr getCurrentTask();

   protected:
    Status m_status = Status::NotDone;
    shared_qobject_ptr<JavaListLoadTask> m_loadTask;
    QList<BaseVersion::Ptr> m_vlist;
};

class JavaListLoadTask : public Task {
    Q_OBJECT

   public:
    explicit JavaListLoadTask(JavaInstallList* vlist);
    virtual ~JavaListLoadTask();

    void executeTask() override;
   public slots:
    void javaCheckerFinished();

   protected:
    shared_qobject_ptr<JavaCheckerJob> m_job;
    JavaInstallList* m_list;
    JavaInstall* m_currentRecommended;
};
