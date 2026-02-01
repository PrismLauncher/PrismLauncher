#pragma once

#include "modplatform/CheckUpdateTask.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Resource*>& resources,
                     std::vector<Version>& mcVersions,
                     QList<ModPlatform::ModLoaderType> loadersList,
                     ResourceFolderModel* resourceModel)
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), resourceModel)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
   private slots:
    void getLatestVersionCallback(Resource* resource, QByteArray* response);
    void collectBlockedMods();

   private:
    Task::Ptr m_task = nullptr;

    QHash<Resource*, QString> m_blocked;
};
