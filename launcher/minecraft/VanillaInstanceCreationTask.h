#pragma once

#include "InstanceCreationTask.h"

#include <utility>

class VanillaCreationTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    VanillaCreationTask(BaseVersion::Ptr version) : InstanceCreationTask(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version(std::move(version)) {}
    VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loader_version);

    bool createInstance() override;

   private:
    // Version to update to / create of the instance.
    BaseVersion::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_using_loader = false;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader;
    BaseVersion::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader_version;
};
