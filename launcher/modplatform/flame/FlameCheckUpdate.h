#pragma once

#include "Application.h"
#include "modplatform/CheckUpdateTask.h"
#include "net/NetJob.h"

class FlameCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    FlameCheckUpdate(QList<Mod*>& mods, std::list<Version>& mcVersions, ModAPI::ModLoaderTypes loaders, std::shared_ptr<ModFolderModel> mods_folder)
        : CheckUpdateTask(mods, mcVersions, loaders, mods_folder)
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;

   private:
    NetJob* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_net_job = nullptr;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_was_aborted = false;
};
