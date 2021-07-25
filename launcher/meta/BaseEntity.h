/* Copyright 2015-2021 MultiMC Contributors
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

#include <QJsonObject>
#include <QObject>
#include "QObjectPtr.h"

#include "net/Mode.h"

class Task;
namespace Meta
{
class BaseEntity
{
public: /* types */
    using Ptr = std::shared_ptr<BaseEntity>;
    enum class LoadStatus
    {
        NotLoaded,
        Local,
        Remote
    };
    enum class UpdateStatus
    {
        NotDone,
        InProgress,
        Failed,
        Succeeded
    };

public:
    virtual ~BaseEntity();

    virtual void parse(const QJsonObject &obj) = 0;

    virtual QString localFilename() const = 0;
    virtual QUrl url() const;

    bool isLoaded() const;
    bool shouldStartRemoteUpdate() const;

    void load(Net::Mode loadType);
    shared_qobject_ptr<Task> getCurrentTask();

protected: /* methods */
    bool loadLocalFile();

private:
    LoadStatus m_loadStatus = LoadStatus::NotLoaded;
    UpdateStatus m_updateStatus = UpdateStatus::NotDone;
    shared_qobject_ptr<Task> m_updateTask;
};
}
