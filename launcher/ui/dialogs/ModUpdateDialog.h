#pragma once

#include "BaseInstance.h"
#include "ModDownloadTask.h"
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

    void appendMod(const CheckUpdateTask::UpdatableMod& info);

    const QList<ModDownloadTask*> getTasks();
    auto indexDir() const -> QDir { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_model->indexDir(); }

    auto noUpdates() const -> bool { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_no_updates; };
    auto aborted() const -> bool { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted; };

   private:
    auto ensureMetadata() -> bool;

   private slots:
    void onMetadataEnsured(Mod*);
    void onMetadataFailed(Mod*, bool try_others = false, ModPlatform::Provider first_choice = ModPlatform::Provider::MODRINTH);

   private:
    QWidget* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent;

    ModrinthCheckUpdate* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_check_task = nullptr;
    FlameCheckUpdate* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_check_task = nullptr;

    const std::shared_ptr<ModFolderModel> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_model;

    QList<Mod*>& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_candidates;
    QList<Mod*> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modrinth_to_update;
    QList<Mod*> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_flame_to_update;

    ConcurrentTask* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_second_try_metadata;
    QList<std::tuple<Mod*, QString>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_metadata;
    QList<std::tuple<Mod*, QString, QUrl>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_check_update;

    QHash<QString, ModDownloadTask*> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks;
    BaseInstance* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_no_updates = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;
};
