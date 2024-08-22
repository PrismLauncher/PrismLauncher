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

#include "net/Mode.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

namespace Meta {
class BaseEntityLoadTask;
class BaseEntity {
    friend BaseEntityLoadTask;

   public: /* types */
    using Ptr = std::shared_ptr<BaseEntity>;
    enum class LoadStatus { NotLoaded, Local, Remote };

   public:
    virtual ~BaseEntity() = default;

    virtual QString localFilename() const = 0;
    virtual QUrl url() const;
    bool isLoaded() const;
    LoadStatus status() const;

    /* for parsers */
    void setSha256(QString sha256);

    virtual void parse(const QJsonObject& obj) = 0;
    [[nodiscard]] Task::Ptr loadTask(Net::Mode loadType = Net::Mode::Online);

   protected:
    QString m_sha256;       // the expected sha256
    QString m_file_sha256;  // the file sha256

   private:
    LoadStatus m_load_status = LoadStatus::NotLoaded;
    Task::Ptr m_task;
};

class BaseEntityLoadTask : public Task {
    Q_OBJECT

   public:
    explicit BaseEntityLoadTask(BaseEntity* parent, Net::Mode mode);
    ~BaseEntityLoadTask() override = default;

    virtual void executeTask() override;
    virtual bool canAbort() const override;
    virtual bool abort() override;

   private:
    BaseEntity* m_entity;
    Net::Mode m_mode;
    NetJob::Ptr m_task;
};
}  // namespace Meta
