#pragma once

#include "settings/SettingsObject.h"
#include "tasks/Task.h"

/* Helpers */
enum class InstanceNameChange { ShouldChange, ShouldKeep };
[[nodiscard]] InstanceNameChange askForChangingInstanceName(QWidget* parent, const QString& old_name, const QString& new_name);
enum class ShouldUpdate { Update, SkipUpdating, Cancel };
[[nodiscard]] ShouldUpdate askIfShouldUpdate(QWidget* parent, QString original_version_name);

struct InstanceName {
   public:
    InstanceName() = default;
    InstanceName(QString name, QString version) : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name(std::move(name)), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version(std::move(version)) {}

    [[nodiscard]] QString modifiedName() const;
    [[nodiscard]] QString originalName() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString version() const;

    void setName(QString name) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name = name; }
    void setName(InstanceName& other);

   protected:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name;
};

class InstanceTask : public Task, public InstanceName {
    Q_OBJECT
   public:
    InstanceTask();
    ~InstanceTask() override = default;

    void setParentSettings(SettingsObjectPtr settings) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings = settings; }

    void setStagingPath(const QString& stagingPath) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath = stagingPath; }

    void setIcon(const QString& icon) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon = icon; }

    void setGroup(const QString& group) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instGroup = group; }
    QString group() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instGroup; }

    [[nodiscard]] bool shouldConfirmUpdate() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update; }
    void setConfirmUpdate(bool confirm) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update = confirm; }

    bool shouldOverride() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_override_existing; }

    [[nodiscard]] QString originalInstanceID() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_instance_id; };

   protected:
    void setOverride(bool override, QString instance_id_to_override = {})
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_override_existing = override;
        if (!instance_id_to_override.isEmpty())
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_instance_id = instance_id_to_override;
    }

   protected: /* data */
    SettingsObjectPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instGroup;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_override_existing = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update = true;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_instance_id;
};
