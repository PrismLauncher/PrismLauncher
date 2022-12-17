#pragma once

#include "minecraft/mod/Mod.h"
#include "modplatform/ModAPI.h"
#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

class ModDownloadTask;
class ModFolderModel;

class CheckUpdateTask : public Task {
    Q_OBJECT

   public:
    CheckUpdateTask(QList<Mod*>& mods, std::list<Version>& mcVersions, ModAPI::ModLoaderTypes loaders, std::shared_ptr<ModFolderModel> mods_folder)
        : Task(nullptr), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods(mods), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_game_versions(mcVersions), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaders(loaders), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_folder(mods_folder) {};

    struct UpdatableMod {
        QString name;
        QString old_hash;
        QString old_version;
        QString new_version;
        QString changelog;
        ModPlatform::Provider provider;
        ModDownloadTask* download;

       public:
        UpdatableMod(QString name, QString old_h, QString old_v, QString new_v, QString changelog, ModPlatform::Provider p, ModDownloadTask* t)
            : name(name), old_hash(old_h), old_version(old_v), new_version(new_v), changelog(changelog), provider(p), download(t)
        {}
    };

    auto getUpdatable() -> std::vector<UpdatableMod>&& { return std::move(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updatable); }

   public slots:
    bool abort() override = 0;

   protected slots:
    void executeTask() override = 0;

   signals:
    void checkFailed(Mod* failed, QString reason, QUrl recover_url = {});

   protected:
    QList<Mod*>& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods;
    std::list<Version>& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_game_versions;
    ModAPI::ModLoaderTypes hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaders;
    std::shared_ptr<ModFolderModel> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_folder;

    std::vector<UpdatableMod> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updatable;
};
