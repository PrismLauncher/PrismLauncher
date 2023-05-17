#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Mod*>& mods,
                     std::list<Version>& mcVersions,
                     std::optional<ResourceAPI::ModLoaderTypes> loaders,
                     std::shared_ptr<ModFolderModel> mods_folder,
                     QStringList blacklist = {})
        : CheckUpdateTask(mods, mcVersions, loaders, mods_folder), m_blacklist(blacklist)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;

   private:
    NetJob* m_net_job = nullptr;
    QStringList m_blacklist = {};

    bool m_was_aborted = false;
};
