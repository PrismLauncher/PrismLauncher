#pragma once

#include "api/structures/Provider.h"
#include "modplatform/CheckUpdateTask.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                        std::list<Version>& mcVersions,
                        QList<Platform::ModLoader> loadersList,
                        std::shared_ptr<ResourceFolderModel> resourceModel)
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), std::move(resourceModel))
        , m_hash_type(Platform::ProviderUtils::hashTypeAlg(Platform::Provider::MODRINTH).first())
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(std::optional<Platform::ModLoaders> loader);
    void checkVersionsResponse(std::shared_ptr<API::GetLatestVersionsResponse> response, std::optional<Platform::ModLoaders> loader);
    void checkNextLoader();

   private:
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    Hashing::Algorithm m_hash_type;
    int m_loader_idx = 0;
};
