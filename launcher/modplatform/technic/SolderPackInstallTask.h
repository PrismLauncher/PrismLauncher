// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2021-2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <InstanceTask.h>
#include <net/NetJob.h>
#include <tasks/Task.h>

#include <QUrl>
#include <memory>

namespace Technic {
class SolderPackInstallTask : public InstanceTask {
    Q_OBJECT
   public:
    explicit SolderPackInstallTask(shared_qobject_ptr<QNetworkAccessManager> network,
                                   const QUrl& solderUrl,
                                   const QString& pack,
                                   const QString& version,
                                   const QString& minecraftVersion);

    bool canAbort() const override { return true; }
    bool abort() override;

   protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

   private slots:
    void fileListSucceeded();
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void downloadProgressChanged(qint64 current, qint64 total);
    void downloadAborted();
    void extractFinished();
    void extractAborted();

   private:
    bool m_abortable = false;

    shared_qobject_ptr<QNetworkAccessManager> m_network;

    NetJob::Ptr m_filesNetJob;
    QUrl m_solderUrl;
    QString m_pack;
    QString m_version;
    QString m_minecraftVersion;
    std::shared_ptr<QByteArray> m_response = std::make_shared<QByteArray>();
    QTemporaryDir m_outputDir;
    int m_modCount;
    QFuture<bool> m_extractFuture;
    QFutureWatcher<bool> m_extractFutureWatcher;
};
}  // namespace Technic
