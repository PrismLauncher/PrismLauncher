#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
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
    NetJob::Ptr m_net_job = nullptr;
};
