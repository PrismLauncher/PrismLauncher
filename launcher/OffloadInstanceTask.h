#pragma once

#include "tasks/Task.h"

#include <QStringList>
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
            QString fileToRemove;
            qint64 fileSize;
        };

        BaseInstance* m_instance;
        std::shared_ptr<Task> m_flameApiJob;

        int m_filesFreed = 0;
        qint64 m_bytesFreed = 0;

        // Maps curseforge file ID to the offload target struct
        QMap<int, CurseForgeOffloadTarget> m_curseForgeFiles;
        QStringList m_disabledFiles;

        void processResourceFolder(const QString& folderName);
        void resolveCurseForgeFiles();
        void finishOffload();

};
