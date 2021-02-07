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

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "QObjectPtr.h"

class QuaZip;
namespace Flame
{
    class FileResolvingTask;
}

class MULTIMC_LOGIC_EXPORT InstanceImportTask : public InstanceTask
{
    Q_OBJECT
public:
    explicit InstanceImportTask(const QUrl sourceUrl);

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private:
    void processZipPack();
    void processMultiMC();
    void processFlame();
    void processTechnic();

private slots:
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void downloadProgressChanged(qint64 current, qint64 total);
    void extractFinished();
    void extractAborted();

private: /* data */
    NetJobPtr m_filesNetJob;
    shared_qobject_ptr<Flame::FileResolvingTask> m_modIdResolver;
    QUrl m_sourceUrl;
    QString m_archivePath;
    bool m_downloadRequired = false;
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<QStringList> m_extractFuture;
    QFutureWatcher<QStringList> m_extractFutureWatcher;
    enum class ModpackType{
        Unknown,
        MultiMC,
        Flame,
        Technic
    } m_modpackType = ModpackType::Unknown;
};
