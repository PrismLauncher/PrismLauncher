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
#include <QtNetwork>
#include "NetAction.h"
#include "Download.h"
#include "HttpMetaCache.h"
#include "tasks/Task.h"
#include "QObjectPtr.h"

#include "multimc_logic_export.h"

class NetJob;
typedef shared_qobject_ptr<NetJob> NetJobPtr;

class MULTIMC_LOGIC_EXPORT NetJob : public Task
{
    Q_OBJECT
public:
    explicit NetJob(QString job_name) : Task()
    {
        setObjectName(job_name);
    }
    virtual ~NetJob();

    bool addNetAction(NetActionPtr action);

    NetActionPtr operator[](int index)
    {
        return downloads[index];
    }
    const NetActionPtr at(const int index)
    {
        return downloads.at(index);
    }
    NetActionPtr first()
    {
        if (downloads.size())
            return downloads[0];
        return NetActionPtr();
    }
    int size() const
    {
        return downloads.size();
    }
    QStringList getFailedFiles();

    bool canAbort() const override;

private slots:
    void startMoreParts();

public slots:
    virtual void executeTask() override;
    virtual bool abort() override;

private slots:
    void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);
    void partSucceeded(int index);
    void partFailed(int index);
    void partAborted(int index);

private:
    struct part_info
    {
        qint64 current_progress = 0;
        qint64 total_progress = 1;
        int failures = 0;
    };
    QList<NetActionPtr> downloads;
    QList<part_info> parts_progress;
    QQueue<int> m_todo;
    QSet<int> m_doing;
    QSet<int> m_done;
    QSet<int> m_failed;
    qint64 m_current_progress = 0;
    bool m_aborted = false;
};
