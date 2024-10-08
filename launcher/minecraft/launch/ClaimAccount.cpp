#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session) : LaunchStep(parent)
{
    if (session->status == AuthSession::Status::PlayableOnline) {
        auto accounts = APPLICATION->accounts();
        m_account = accounts->getAccountByProfileName(session->player_name);
    }
}

void ClaimAccount::executeTask()
{
    if (m_account) {
        lock.reset(new UseLock(m_account));
        emitSucceeded();
    }
}

void ClaimAccount::finalize()
{
    lock.reset();
}
