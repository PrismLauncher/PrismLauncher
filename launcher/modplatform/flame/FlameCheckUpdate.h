#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "modplatform/ModIndex.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Resource*>& resources,
                     std::vector<Version>& mcVersions,
                     QList<ModPlatform::ModLoaderType> loadersList,
                     ResourceFolderModel* resourceModel,
                     std::vector<ModPlatform::IndexedVersionType> releaseTypes = {})
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), resourceModel, std::move(releaseTypes))
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
   private slots:
    void getLatestVersionCallback(Resource* resource, QList<ModPlatform::IndexedVersion>* response);
    void collectBlockedMods();

   private:
    Task::Ptr m_task = nullptr;

    QHash<Resource*, QString> m_blocked;
};
