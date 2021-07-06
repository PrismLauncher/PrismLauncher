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

#include <InstanceTask.h>
#include <net/NetJob.h>
#include <tasks/Task.h>

#include <QUrl>

namespace Technic
{
    class MULTIMC_LOGIC_EXPORT SolderPackInstallTask : public InstanceTask
    {
        Q_OBJECT
    public:
        explicit SolderPackInstallTask(const QUrl &sourceUrl, const QString &minecraftVersion);

        bool canAbort() const override { return true; }
        bool abort() override;

    protected:
        //! Entry point for tasks.
        virtual void executeTask() override;

    private slots:
        void versionSucceeded();
        void fileListSucceeded();
        void downloadSucceeded();
        void downloadFailed(QString reason);
        void downloadProgressChanged(qint64 current, qint64 total);
        void extractFinished();
        void extractAborted();

    private:
        bool m_abortable = false;

        NetJobPtr m_filesNetJob;
        QUrl m_sourceUrl;
        QString m_minecraftVersion;
        QByteArray m_response;
        QTemporaryDir m_outputDir;
        int m_modCount;
        QFuture<bool> m_extractFuture;
        QFutureWatcher<bool> m_extractFutureWatcher;
    };
}
