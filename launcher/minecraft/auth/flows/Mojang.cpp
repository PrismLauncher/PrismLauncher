#include "Mojang.h"

#include "minecraft/auth/steps/GetSkinStep.h"
#include "minecraft/auth/steps/MigrationEligibilityStep.h"
#include "minecraft/auth/steps/MinecraftProfileStepMojang.h"
#include "minecraft/auth/steps/YggdrasilStep.h"

MojangRefresh::MojangRefresh(AccountData* data, QObject* parent) : AuthFlow(data, parent)
{
    m_steps.append(makeShared<YggdrasilStep>(m_data, QString()));
    m_steps.append(makeShared<MinecraftProfileStepMojang>(m_data));
    m_steps.append(makeShared<MigrationEligibilityStep>(m_data));
    m_steps.append(makeShared<GetSkinStep>(m_data));
}

MojangLogin::MojangLogin(AccountData* data, QString password, QObject* parent) : AuthFlow(data, parent), m_password(password)
{
    m_steps.append(makeShared<YggdrasilStep>(m_data, m_password));
    m_steps.append(makeShared<MinecraftProfileStepMojang>(m_data));
    m_steps.append(makeShared<MigrationEligibilityStep>(m_data));
    m_steps.append(makeShared<GetSkinStep>(m_data));
}
