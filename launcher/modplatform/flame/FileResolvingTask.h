#pragma once

#include "PackManifest.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

namespace Flame {
class FileResolvingTask : public Task {
    Q_OBJECT
   public:
    explicit FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess);
    virtual ~FileResolvingTask(){};

    bool canAbort() const override { return true; }
    bool abort() override;

    const Flame::Manifest& getResults() const { return m_toProcess; }

   protected:
    virtual void executeTask() override;

   protected slots:
    void netJobFinished();

   private: /* data */
    shared_qobject_ptr<QNetworkAccessManager> m_network;
    Flame::Manifest m_toProcess;
    std::shared_ptr<QByteArray> result;
    NetJob::Ptr m_dljob;
    NetJob::Ptr m_checkJob;
    NetJob::Ptr m_slugJob;

    void modrinthCheckFinished();

    QMap<File*, std::shared_ptr<QByteArray>> blockedProjects;
};
}  // namespace Flame
