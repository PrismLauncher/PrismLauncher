#include "YggdrasilStep.h"

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "minecraft/auth/Yggdrasil.h"

YggdrasilStep::YggdrasilStep(AccountData* data, QString password) : AuthStep(data), m_password(password)
{
    m_yggdrasil = new Yggdrasil(m_data, this);

    connect(m_yggdrasil, &Task::failed, this, &YggdrasilStep::onAuthFailed);
    connect(m_yggdrasil, &Task::succeeded, this, &YggdrasilStep::onAuthSucceeded);
    connect(m_yggdrasil, &Task::aborted, this, &YggdrasilStep::onAuthFailed);
}

YggdrasilStep::~YggdrasilStep() noexcept = default;

QString YggdrasilStep::describe()
{
    return tr("Logging in with Mojang account.");
}

void YggdrasilStep::rehydrate()
{
    // NOOP, for now.
}

void YggdrasilStep::perform()
{
    if (m_password.size()) {
        m_yggdrasil->login(m_password);
    } else {
        m_yggdrasil->refresh();
    }
}

void YggdrasilStep::onAuthSucceeded()
{
    emit finished(AccountTaskState::STATE_WORKING, tr("Logged in with Mojang"));
}

void YggdrasilStep::onAuthFailed()
{
    // TODO: hook these in again, expand to MSA
    // m_error = m_yggdrasil->m_error;
    // m_aborted = m_yggdrasil->m_aborted;

    auto state = m_yggdrasil->taskState();
    QString errorMessage = tr("Mojang user authentication failed.");

    // NOTE: soft error in the first step means 'offline'
    if (state == AccountTaskState::STATE_FAILED_SOFT) {
        state = AccountTaskState::STATE_OFFLINE;
        errorMessage = tr("Mojang user authentication ended with a network error.");
    }
    emit finished(state, errorMessage);
}
