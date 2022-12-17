#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session): LaunchStep(parent)
{
    if(session->status == AuthSession::Status::PlayableOnline && !session->demo)
    {
        auto accounts = APPLICATION->accounts();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account = accounts->getAccountByProfileName(session->player_name);
    }
}

void ClaimAccount::executeTask()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account)
    {
        lock.reset(new UseLock(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account));
        emitSucceeded();
    }
}

void ClaimAccount::finalize()
{
    lock.reset();
}
