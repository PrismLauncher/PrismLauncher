#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Resource*>& resources,
                     std::list<Version>& mcVersions,
                     QList<ModPlatform::ModLoaderType> loadersList,
                     std::shared_ptr<ResourceFolderModel> resourceModel)
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), std::move(resourceModel))
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
