#pragma once

#include "BaseInstance.h"
#include "ModDownloadTask.h"
#include "ReviewMessageBox.h"

#include "minecraft/mod/ModFolderModel.h"

#include "modplatform/CheckUpdateTask.h"

class Mod;
class ModrinthCheckUpdate;
class FlameCheckUpdate;

class ModUpdateDialog final : public ReviewMessageBox {
    Q_OBJECT
   public:
    explicit ModUpdateDialog(QWidget* parent,
                             BaseInstance* instance,
                             const std::shared_ptr<ModFolderModel> mod_model,
                             std::list<Mod>& search_for);

    void checkCandidates();

    void appendMod(const CheckUpdateTask::UpdatableMod& info);

    const std::list<ModDownloadTask*> getTasks();
    auto indexDir() const -> QDir { return m_mod_model->indexDir(); }

    auto noUpdates() const -> bool { return m_no_updates; };
    auto aborted() const -> bool { return m_aborted; };

   private:
    auto ensureMetadata() -> bool;

   private slots:
    void onMetadataEnsured(Mod&);
    void onMetadataFailed(Mod&, bool try_others = false, ModPlatform::Provider first_choice = ModPlatform::Provider::MODRINTH);

   private:
    QWidget* m_parent;

    ModrinthCheckUpdate* m_modrinth_check_task = nullptr;
    FlameCheckUpdate* m_flame_check_task = nullptr;

    const std::shared_ptr<ModFolderModel> m_mod_model;

    std::list<Mod>& m_candidates;
    std::list<Mod> m_modrinth_to_update;
    std::list<Mod> m_flame_to_update;

    SequentialTask* m_second_try_metadata;
    std::list<std::tuple<Mod, QString>> m_failed_metadata;
    std::list<std::tuple<Mod, QString, QUrl>> m_failed_check_update;

    QHash<QString, ModDownloadTask*> m_tasks;
    BaseInstance* m_instance;

    bool m_no_updates = false;
    bool m_aborted = false;
};
