#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Mod*>& mods,
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
    NetJob* m_net_job = nullptr;

    bool m_was_aborted = false;
};
