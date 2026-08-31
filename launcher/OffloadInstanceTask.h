#pragma once

#include "tasks/Task.h"

#include <QMap>
#include <memory>
class BaseInstance;

class OffloadInstanceTask : public Task
{
     Q_OBJECT
    public:
        explicit OffloadInstanceTask(BaseInstance* instance);
        ~OffloadInstanceTask() override = default;

    protected:
        void executeTask() override;

    private:

        struct CurseForgeOffloadTarget
        {
            int projectId;
            QString targetFile;
            qint64 fileSize;
        };
        void processResourceFolder(const QString& folderName);
        void resolveCurseForgeFiles();
        void finishOffload();

        BaseInstance* m_instance;
        std::shared_ptr<Task> m_flameApiJob;

        int m_filesFreed = 0;
        qint64 m_bytesFreed = 0;

        // Maps curseforge file ID to the offload target struct
        QMap<int, CurseForgeOffloadTarget> m_curseForgeFiles;
};
