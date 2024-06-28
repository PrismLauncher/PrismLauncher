#pragma once

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"
#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

class ResourceDownloadTask;
class ModFolderModel;

class CheckUpdateTask : public Task {
    Q_OBJECT

   public:
    CheckUpdateTask(QList<Mod*>& mods,
                    std::list<Version>& mcVersions,
                    QList<ModPlatform::ModLoaderType> loadersList,
                    std::shared_ptr<ModFolderModel> mods_folder)
        : Task(nullptr), m_mods(mods), m_game_versions(mcVersions), m_loaders_list(loadersList), m_mods_folder(mods_folder) {};

    struct UpdatableMod {
        QString name;
        QString old_hash;
        QString old_version;
        QString new_version;
        std::optional<ModPlatform::IndexedVersionType> new_version_type;
        QString changelog;
        ModPlatform::ResourceProvider provider;
        shared_qobject_ptr<ResourceDownloadTask> download;
        bool enabled = true;

       public:
        UpdatableMod(QString name,
                     QString old_h,
                     QString old_v,
                     QString new_v,
                     std::optional<ModPlatform::IndexedVersionType> new_v_type,
                     QString changelog,
                     ModPlatform::ResourceProvider p,
                     shared_qobject_ptr<ResourceDownloadTask> t,
                     bool enabled = true)
            : name(name)
            , old_hash(old_h)
            , old_version(old_v)
            , new_version(new_v)
            , new_version_type(new_v_type)
            , changelog(changelog)
            , provider(p)
            , download(t)
            , enabled(enabled)
        {}
    };

    auto getUpdatable() -> std::vector<UpdatableMod>&& { return std::move(m_updatable); }
    auto getDependencies() -> QList<std::shared_ptr<GetModDependenciesTask::PackDependency>>&& { return std::move(m_deps); }

   public slots:
    bool abort() override = 0;

   protected slots:
    void executeTask() override = 0;

   signals:
    void checkFailed(Mod* failed, QString reason, QUrl recover_url = {});

   protected:
    QList<Mod*>& m_mods;
    std::list<Version>& m_game_versions;
    QList<ModPlatform::ModLoaderType> m_loaders_list;
    std::shared_ptr<ModFolderModel> m_mods_folder;

    std::vector<UpdatableMod> m_updatable;
    QList<std::shared_ptr<GetModDependenciesTask::PackDependency>> m_deps;
};
