#pragma once

#include "PackManifest.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

namespace Flame {
class FileResolvingTask : public TaskV2 {
    Q_OBJECT
   public:
    explicit FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess);
    virtual ~FileResolvingTask() {};

    const Flame::Manifest& getResults() const { return m_toProcess; }

   protected slots:
    bool doAbort() override;
    virtual void executeTask() override;

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
