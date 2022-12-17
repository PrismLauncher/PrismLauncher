#include "Offline.h"

#include "minecraft/auth/steps/OfflineStep.h"

OfflineRefresh::OfflineRefresh(
    AccountData *data,
    QObject *parent
) : AuthFlow(data, parent) {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps.append(new OfflineStep(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data));
}

OfflineLogin::OfflineLogin(
    AccountData *data,
    QObject *parent
) : AuthFlow(data, parent) {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps.append(new OfflineStep(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data));
}
