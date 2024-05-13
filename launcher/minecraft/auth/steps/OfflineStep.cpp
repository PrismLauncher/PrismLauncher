#include "OfflineStep.h"

OfflineStep::OfflineStep(AccountData* data) : AuthStep(data) {}

QString OfflineStep::describe()
{
    return tr("Creating offline account.");
}

void OfflineStep::perform()
{
    emit finished(AccountTaskState::STATE_WORKING, tr("Created offline account."));
}
