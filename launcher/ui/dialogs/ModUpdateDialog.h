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

class ModUpdateDialog final : public ReviewMessageBox {
    Q_OBJECT
   public:
    explicit ModUpdateDialog(QWidget* parent,
                             BaseInstance* instance,
                             const std::shared_ptr<ModFolderModel> mod_model,
                             QList<Mod*>& search_for);

    void checkCandidates();

    void appendMod(const CheckUpdateTask::UpdatableMod& info, QStringList requiredBy = {});

    const QList<ResourceDownloadTask::Ptr> getTasks();
    auto indexDir() const -> QDir { return m_mod_model->indexDir(); }

    auto noUpdates() const -> bool { return m_no_updates; };
    auto aborted() const -> bool { return m_aborted; };

   private:
    auto ensureMetadata() -> bool;

   private slots:
    void onMetadataEnsured(Mod*);
    void onMetadataFailed(Mod*,
                          bool try_others = false,
                          ModPlatform::ResourceProvider first_choice = ModPlatform::ResourceProvider::MODRINTH);

   private:
    QWidget* m_parent;

    shared_qobject_ptr<ModrinthCheckUpdate> m_modrinth_check_task;
    shared_qobject_ptr<FlameCheckUpdate> m_flame_check_task;

    const std::shared_ptr<ModFolderModel> m_mod_model;

    QList<Mod*>& m_candidates;
    QList<Mod*> m_modrinth_to_update;
    QList<Mod*> m_flame_to_update;

    ConcurrentTask::Ptr m_second_try_metadata;
    QList<std::tuple<Mod*, QString>> m_failed_metadata;
    QList<std::tuple<Mod*, QString, QUrl>> m_failed_check_update;

    QHash<QString, ResourceDownloadTask::Ptr> m_tasks;
    BaseInstance* m_instance;

    bool m_no_updates = false;
    bool m_aborted = false;
};
