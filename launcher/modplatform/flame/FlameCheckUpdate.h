#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "modplatform/flame/FlameAPI.h"

class NetworkContext;

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Resource*>& resources,
                     std::vector<Version>& mcVersions,
                     QList<ModPlatform::ModLoaderType> loadersList,
                     ResourceFolderModel* resourceModel,
                     NetworkContext* network)
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), resourceModel), m_api(network)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
   private slots:
    void getLatestVersionCallback(Resource* resource, QByteArray* response);
    void collectBlockedMods();

   private:
    FlameAPI m_api;
    Task::Ptr m_task = nullptr;

    QHash<Resource*, QString> m_blocked;
};
