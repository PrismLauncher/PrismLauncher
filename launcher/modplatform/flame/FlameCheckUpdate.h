#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Resource*>& resources,
                     std::list<Version>& mcVersions,
                     std::optional<ModPlatform::ModLoaderTypes> loaders,
                     std::shared_ptr<ResourceFolderModel> resource_model)
        : CheckUpdateTask(resources, mcVersions, loaders, resource_model)
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
