#pragma once

#include "modplatform/flame/PackManifest.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

#include <QSet>
#include <QStringList>

class BaseInstance;

class RestoreInstanceTask : public Task {
    Q_OBJECT
   public:
    explicit RestoreInstanceTask(BaseInstance* instance);
    ~RestoreInstanceTask() override = default;

   protected:
    void executeTask() override;

    bool abort() override;

   private:
    BaseInstance* m_instance;
    NetJob::Ptr m_netJob;
    std::shared_ptr<Task> m_resolveTask;

    QStringList m_disabledFilenames;
    QSet<int> m_disabledCFFileIds;

    void restoreResourceFolder(const QString& folderName, Flame::Manifest& manifest);
    void resolveCurseForgeFiles(Flame::Manifest& manifest);
    void startDownloadJob();
};
