/* Copyright 2013-2020 MultiMC Contributors
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

#ifndef TECHNIC_SINGLEZIPPACKINSTALLTASK_H
#define TECHNIC_SINGLEZIPPACKINSTALLTASK_H

#include "InstanceTask.h"
#include "net/NetJob.h"
#include "multimc_logic_export.h"

#include "quazip.h"

#include <QFutureWatcher>
#include <QStringList>
#include <QUrl>

namespace Technic {

class MULTIMC_LOGIC_EXPORT SingleZipPackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    SingleZipPackInstallTask(const QUrl &sourceUrl, const QString &minecraftVersion);

protected:
    void executeTask() override;


private slots:
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void downloadProgressChanged(qint64 current, qint64 total);
    void extractFinished();
    void extractAborted();

private:
    QUrl m_sourceUrl;
    QString m_minecraftVersion;
    QString m_archivePath;
    NetJobPtr m_filesNetJob;
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<QStringList> m_extractFuture;
    QFutureWatcher<QStringList> m_extractFutureWatcher;
};

} // namespace Technic

#endif // TECHNIC_SINGLEZIPPACKINSTALLTASK_H
