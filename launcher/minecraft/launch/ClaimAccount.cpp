#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include <memory>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, const AuthSessionPtr& session) : LaunchStep(parent)
{
    if (session->launchMode == LaunchMode::Normal) {
        auto accounts = APPLICATION->accounts();
        m_account = accounts->getAccountByProfileName(session->player_name);
    }
}

void ClaimAccount::executeTask()
{
    if (m_account) {
        lock = std::make_unique<UseLock>(m_account.get());
    }
    emitSucceeded();
}

void ClaimAccount::finalize()
{
    lock.reset();
}
