#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "modplatform/ModIndex.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                        std::vector<Version>& mcVersions,
                        QList<ModPlatform::ModLoaderType> loadersList,
                        ResourceFolderModel* resourceModel);

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(std::optional<ModPlatform::ModLoaderTypes> loader = {}, bool forceModLoaderCheck = false);
    void checkVersionsResponse(const QHash<QString, ModPlatform::IndexedVersion>& versions,
                               std::optional<ModPlatform::ModLoaderTypes> loader);
    void checkNextLoader();

   private:
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    QString m_hashType;
    int m_loaderIdx = 0;
    qsizetype m_initialSize = 0;
};
