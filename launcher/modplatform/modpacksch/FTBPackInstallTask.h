/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackManifest.h"

#include "QObjectPtr.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "InstanceTask.h"
#include "net/NetJob.h"

#include <QWidget>

namespace ModpacksCH {

class PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version, QWidget* parent = nullptr);
    virtual ~PackInstallTask(){}

    bool canAbort() const override { return true; }
    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    void resolveMods();
    void downloadPack();
    void install();

private:
    bool abortable = false;

    NetJob::Ptr jobPtr;
    shared_qobject_ptr<Flame::FileResolvingTask> modIdResolver;
    QMap<int, int> indexFileIdMap;

    QByteArray response;

    Modpack m_pack;
    QString m_version_name;
    Version m_version;

    QMap<QString, QString> filesToCopy;

    //FIXME: nuke
    QWidget* m_parent;
};

}
