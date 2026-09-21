// SPDX-License-Identifier: GPL-3.0-only
#include "SharedContentStep.h"

#include "Application.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "shared/SharedContentManager.h"

SharedContentStep::SharedContentStep(LaunchTask* parent) : LaunchStep(parent) {}

void SharedContentStep::executeTask()
{
    auto* application = APPLICATION_DYN;
    auto* manager = application ? application->sharedContent() : nullptr;
    auto* instance = m_parent->instance();

    if (!manager || manager->instanceGroup(instance).isEmpty()) {
        emitSucceeded();
        return;
    }

    QString error;
    if (!manager->prepare(instance, &error)) {
        manager->cancel(instance);
        emit logLine(tr("Could not prepare shared content: %1").arg(error), MessageLevel::Error);
        emitFailed(tr("Could not prepare shared content: %1").arg(error));
        return;
    }

    m_prepared = true;
    emit logLine(tr("Shared content group '%1' is ready.").arg(manager->instanceGroup(instance)), MessageLevel::Launcher);
    emitSucceeded();
}

void SharedContentStep::finalize()
{
    if (!m_prepared) {
        return;
    }

    auto* application = APPLICATION_DYN;
    auto* manager = application ? application->sharedContent() : nullptr;
    if (!manager) {
        return;
    }

    QString error;
    if (!manager->finish(m_parent->instance(), &error)) {
        emit logLine(tr("Could not save shared content changes: %1").arg(error), MessageLevel::Error);
    }
    m_prepared = false;
}
