#include "YggdrasilStep.h"

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "minecraft/auth/Yggdrasil.h"

YggdrasilStep::YggdrasilStep(AccountData* data, QString password) : AuthStep(data), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_password(password) {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil = new Yggdrasil(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data, this);

    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil, &Task::failed, this, &YggdrasilStep::onAuthFailed);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil, &Task::succeeded, this, &YggdrasilStep::onAuthSucceeded);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil, &Task::aborted, this, &YggdrasilStep::onAuthFailed);
}

YggdrasilStep::~YggdrasilStep() noexcept = default;

QString YggdrasilStep::describe() {
    return tr("Logging in with Mojang account.");
}

void YggdrasilStep::rehydrate() {
    // NOOP, for now.
}

void YggdrasilStep::perform() {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_password.size()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil->login(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_password);
    }
    else {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil->refresh();
    }
}

void YggdrasilStep::onAuthSucceeded() {
    emit finished(AccountTaskState::STATE_WORKING, tr("Logged in with Mojang"));
}

void YggdrasilStep::onAuthFailed() {
    // TODO: hook these in again, expand to MSA
    // hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_error = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_error;
    // hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted;

    auto state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil->taskState();
    QString errorMessage = tr("Mojang user authentication failed.");

    // NOTE: soft error in the first step means 'offline'
    if(state == AccountTaskState::STATE_FAILED_SOFT) {
        state = AccountTaskState::STATE_OFFLINE;
        errorMessage = tr("Mojang user authentication ended with a network error.");
    }
    emit finished(state, errorMessage);
}
