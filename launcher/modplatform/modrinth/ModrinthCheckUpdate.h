#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "tasks/Task.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Mod*>& mods,
                        std::list<Version>& mcVersions,
                        std::optional<ModPlatform::ModLoaderTypes> loaders,
                        std::shared_ptr<ModFolderModel> mods_folder)
        : CheckUpdateTask(mods, mcVersions, loaders, mods_folder)
    {}

   protected slots:
    bool doAbort() override;
    void executeTask() override;
    void hashTaskFinished();

   private:
    TaskV2::Ptr m_net_job = nullptr;
    QHash<QString, Mod*> m_mappings;
    QStringList hashes;
};
