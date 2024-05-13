#include "Offline.h"

#include "minecraft/auth/steps/OfflineStep.h"

OfflineLogin::OfflineLogin(AccountData* data, QObject* parent) : AuthFlow(data, parent)
{
    m_steps.append(makeShared<OfflineStep>(m_data));
}
