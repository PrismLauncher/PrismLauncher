#pragma once

#include "QObjectPtr.h"
#include "minecraft/mod/LocalModUpdateTask.h"
#include "modplatform/ModIndex.h"
#include "tasks/SequentialTask.h"
#include "minecraft/mod/ModFolderModel.h"
#include "net/NetJob.h"
#include <QUrl>

class ModDownloadTask : public SequentialTask {
    Q_OBJECT
public:
    explicit ModDownloadTask(ModPlatform::IndexedPack mod, ModPlatform::IndexedVersion version, const std::shared_ptr<ModFolderModel> mods);
    const QString& getFilename() const { return m_mod_version.fileName; }

private:
    ModPlatform::IndexedPack m_mod;
    ModPlatform::IndexedVersion m_mod_version;
    const std::shared_ptr<ModFolderModel> mods;

    NetJob::Ptr m_filesNetJob;
    LocalModUpdateTask::Ptr m_update_task;

    void downloadProgressChanged(qint64 current, qint64 total);

    void downloadFailed(QString reason);

    void downloadSucceeded();
};



