#pragma once

#include "minecraft/ModLoaderType.h"
#include "modplatform/CheckUpdateTask.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                        std::vector<Version>& mcVersions,
                        QSet<ModLoaderType> loadersList,
                        ResourceFolderModel* resourceModel,
                        std::vector<ModPlatform::IndexedVersionType> releaseTypes = {});

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(std::optional<ModLoaderType> loader = {}, bool forceModLoaderCheck = false);
    void checkVersionsResponse(QByteArray* response, std::optional<ModLoaderType> loader);
    void checkNextLoader();

   private:
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    QString m_hashType;
    QList<ModLoaderType> m_tryList;
    int m_loaderIdx = 0;
    qsizetype m_initialSize = 0;
};
