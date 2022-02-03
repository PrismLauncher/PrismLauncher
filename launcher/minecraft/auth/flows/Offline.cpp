#include "Offline.h"

#include "minecraft/auth/steps/OfflineStep.h"

OfflineRefresh::OfflineRefresh(
    AccountData *data,
    QObject *parent
) : AuthFlow(data, parent) {
    m_steps.append(new OfflineStep(m_data));
}

OfflineLogin::OfflineLogin(
    AccountData *data,
    QObject *parent
) : AuthFlow(data, parent) {
    m_steps.append(new OfflineStep(m_data));
}
