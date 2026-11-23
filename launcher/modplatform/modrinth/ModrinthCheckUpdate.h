#pragma once

#include "modplatform/CheckUpdateTask.h"
#include "modplatform/modrinth/ModrinthAPI.h"

class LauncherSettings;
class NetworkContext;

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                        std::vector<Version>& mcVersions,
                        QList<ModPlatform::ModLoaderType> loadersList,
                        ResourceFolderModel* resourceModel,
                        NetworkContext* network,
                        LauncherSettings* settings);

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(std::optional<ModPlatform::ModLoaderTypes> loader = {}, bool forceModLoaderCheck = false);
    void checkVersionsResponse(QByteArray* response, std::optional<ModPlatform::ModLoaderTypes> loader);
    void checkNextLoader();

   private:
    ModrinthAPI m_api;
    LauncherSettings* m_settings = nullptr;
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    QString m_hashType;
    int m_loaderIdx = 0;
    qsizetype m_initialSize = 0;
};
