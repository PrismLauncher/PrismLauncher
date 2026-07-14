#pragma once

#include "minecraft/mod/tasks/GetModDependenciesTask.h"
#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

class ResourceDownloadTask;
class ModFolderModel;

class CheckUpdateTask : public Task {
    Q_OBJECT

   public:
    CheckUpdateTask(QList<Resource*>& resources,
                    std::vector<Version>& mcVersions,
                    QList<ModPlatform::ModLoaderType> loadersList,
                    ResourceFolderModel* resourceModel)
        : m_resources(resources), m_gameVersions(mcVersions), m_loadersList(std::move(loadersList)), m_resourceModel(resourceModel)
    {}

    struct Update {
        QString name;
        QString oldHash;
        QString oldVersion;
        QString newVersion;
        std::optional<ModPlatform::IndexedVersionType> newVersionType;
        QString changelog;
        ModPlatform::ResourceProvider provider;
        shared_qobject_ptr<ResourceDownloadTask> download;
        bool enabled = true;

       public:
        Update(QString name,
               QString oldH,
               QString oldV,
               QString newV,
               std::optional<ModPlatform::IndexedVersionType> newVType,
               QString changelog,
               ModPlatform::ResourceProvider p,
               shared_qobject_ptr<ResourceDownloadTask> t,
               bool enabled = true)
            : name(std::move(name))
            , oldHash(std::move(oldH))
            , oldVersion(std::move(oldV))
            , newVersion(std::move(newV))
            , newVersionType(newVType)
            , changelog(std::move(changelog))
            , provider(p)
            , download(std::move(t))
            , enabled(enabled)
        {}
    };

    auto getUpdates() -> std::vector<Update>&& { return std::move(m_updates); }
    auto getDependencies() -> QList<std::shared_ptr<GetModDependenciesTask::PackDependency>>&& { return std::move(m_deps); }

   protected slots:
    void executeTask() override = 0;

   signals:
    void checkFailed(Resource* failed, QString reason, QUrl recoverUrl = {});

   protected:
    QList<Resource*>& m_resources;
    std::vector<Version>& m_gameVersions;
    QList<ModPlatform::ModLoaderType> m_loadersList;
    ResourceFolderModel* m_resourceModel;

    std::vector<Update> m_updates;
    QList<std::shared_ptr<GetModDependenciesTask::PackDependency>> m_deps;
};
