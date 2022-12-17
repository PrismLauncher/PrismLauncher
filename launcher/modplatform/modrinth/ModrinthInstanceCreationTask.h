#pragma once

#include "InstanceCreationTask.h"

#include <optional>

#include "minecraft/MinecraftInstance.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"

#include "net/NetJob.h"

class ModrinthCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    ModrinthCreationTask(QString staging_path,
                         SettingsObjectPtr global_settings,
                         QWidget* parent,
                         QString id,
                         QString version_id = {},
                         QString original_instance_id = {})
        : InstanceCreationTask()
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent(parent)
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_id(std::move(id))
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_version_id(std::move(version_id))
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_instance_id = std::move(original_instance_id);
    }

    bool abort() override;

    bool updateInstance() override;
    bool createInstance() override;

   private:
    bool parseManifest(const QString&, std::vector<Modrinth::File>&, bool set_internal_data = true, bool show_optional_dialog = true);

   private:
    QWidget* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = nullptr;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraft_version, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fabric_version, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_quilt_version, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_forge_version;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_id, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_version_id, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_name;

    std::vector<Modrinth::File> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files;
    NetJob::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files_job;

    std::optional<InstancePtr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;
};
