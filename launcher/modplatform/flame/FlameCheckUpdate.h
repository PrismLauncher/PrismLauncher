#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Mod*>& mods,
                     std::list<Version>& mcVersions,
                     QList<ModPlatform::ModLoaderType> loadersList,
                     std::shared_ptr<ModFolderModel> mods_folder)
        : CheckUpdateTask(mods, mcVersions, loadersList, mods_folder)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;

   private:
    ModPlatform::IndexedPack getProjectInfo(ModPlatform::IndexedVersion& ver_info);
    ModPlatform::IndexedVersion getFileInfo(int addonId, int fileId);

    NetJob* m_net_job = nullptr;

    bool m_was_aborted = false;
};
