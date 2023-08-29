#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Mod*>& mods,
                        std::list<Version>& mcVersions,
                        std::optional<ModPlatform::ModLoaderTypes> loaders,
                        std::shared_ptr<ModFolderModel> mods_folder)
        : CheckUpdateTask(mods, mcVersions, loaders, mods_folder)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;

   private:
    NetJob::Ptr m_net_job = nullptr;
};
