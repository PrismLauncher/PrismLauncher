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
#include "net/NetJob.h"

#include <quazip/quazip.h>

#include <QFutureWatcher>
#include <QStringList>
#include <QUrl>

#include <optional>

namespace Technic {

class SingleZipPackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    SingleZipPackInstallTask(const QUrl &sourceUrl, const QString &minecraftVersion);

    bool canAbort() const override { return true; }
    bool abort() override;

protected:
    void executeTask() override;


private slots:
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void downloadProgressChanged(qint64 current, qint64 total);
    void extractFinished();
    void extractAborted();

private:
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = false;

    QUrl hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_archivePath;
    NetJob::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob;
    std::unique_ptr<QuaZip> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip;
    QFuture<std::optional<QStringList>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture;
    QFutureWatcher<std::optional<QStringList>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher;
};

} // namespace Technic
