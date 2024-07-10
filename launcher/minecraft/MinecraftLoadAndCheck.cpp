#include "MinecraftLoadAndCheck.h"
#include "MinecraftInstance.h"
#include "PackProfile.h"

MinecraftLoadAndCheck::MinecraftLoadAndCheck(MinecraftInstance* inst, QObject* parent) : TaskV2(parent), m_inst(inst) {}

void MinecraftLoadAndCheck::executeTask()
{
    // add offline metadata load task
    auto components = m_inst->getPackProfile();
    components->reload(Net::Mode::Offline);
    m_task = components->getCurrentTask();

    if (!m_task) {
        emitSucceeded();
        return;
    }
    connect(m_task.get(), &TaskV2::finished, this, &MinecraftLoadAndCheck::finished);
    connect(m_task.get(), &TaskV2::totalChanged, this, &MinecraftLoadAndCheck::totalChanged);
    connect(m_task.get(), &TaskV2::processedChanged, this, &MinecraftLoadAndCheck::processedChanged);
    connect(m_task.get(), &TaskV2::weightChanged, this, &MinecraftLoadAndCheck::weightChanged);
    connect(m_task.get(), &TaskV2::stateChanged, this, &MinecraftLoadAndCheck::stateChanged);
}
