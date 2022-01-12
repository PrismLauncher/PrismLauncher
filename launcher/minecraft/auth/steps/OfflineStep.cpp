#include "OfflineStep.h"

#include "Application.h"

OfflineStep::OfflineStep(AccountData* data) : AuthStep(data) {}
OfflineStep::~OfflineStep() noexcept = default;

QString OfflineStep::describe() {
    return tr("Creating offline account.");
}

void OfflineStep::rehydrate() {
    // NOOP
}

void OfflineStep::perform() {
    emit finished(AccountTaskState::STATE_WORKING, tr("Created offline account."));
}
