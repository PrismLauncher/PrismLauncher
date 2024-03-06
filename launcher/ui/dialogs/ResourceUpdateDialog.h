#pragma once

#include "BaseInstance.h"
#include "ResourceDownloadTask.h"
#include "ReviewMessageBox.h"

#include "minecraft/mod/ModFolderModel.h"

#include "modplatform/CheckUpdateTask.h"

class Mod;
class ModrinthCheckUpdate;
class FlameCheckUpdate;
class ConcurrentTask;

class ResourceUpdateDialog final : public ReviewMessageBox {
    Q_OBJECT
   public:
    explicit ResourceUpdateDialog(QWidget* parent,
                                  BaseInstance* instance,
                                  std::shared_ptr<ResourceFolderModel> resource_model,
                                  QList<Resource*>& search_for,
                                  bool include_deps,
                                  bool filter_loaders);

    void checkCandidates();

    void appendResource(const CheckUpdateTask::Update& info, QStringList requiredBy = {});

    const QList<ResourceDownloadTask::Ptr> getTasks();
    auto indexDir() const -> QDir { return m_resource_model->indexDir(); }

    auto noUpdates() const -> bool { return m_no_updates; };
    auto aborted() const -> bool { return m_aborted; };

   private:
    auto ensureMetadata() -> bool;

   private slots:
    void onMetadataEnsured(Resource* resource);
    void onMetadataFailed(Resource* resource,
                          bool try_others = false,
                          ModPlatform::ResourceProvider first_choice = ModPlatform::ResourceProvider::MODRINTH);

   private:
    QWidget* m_parent;

    shared_qobject_ptr<ModrinthCheckUpdate> m_modrinth_check_task;
    shared_qobject_ptr<FlameCheckUpdate> m_flame_check_task;

    const std::shared_ptr<ResourceFolderModel> m_resource_model;

    QList<Resource*>& m_candidates;
    QList<Resource*> m_modrinth_to_update;
    QList<Resource*> m_flame_to_update;

    ConcurrentTask::Ptr m_second_try_metadata;
    QList<std::tuple<Resource*, QString>> m_failed_metadata;
    QList<std::tuple<Resource*, QString, QUrl>> m_failed_check_update;

    QHash<QString, ResourceDownloadTask::Ptr> m_tasks;
    BaseInstance* m_instance;

    bool m_no_updates = false;
    bool m_aborted = false;
    bool m_include_deps = false;
    bool m_filter_loaders = false;
};
