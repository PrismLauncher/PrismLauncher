#include "MSA.h"

#include "minecraft/auth/steps/EntitlementsStep.h"
#include "minecraft/auth/steps/GetSkinStep.h"
#include "minecraft/auth/steps/LauncherLoginStep.h"
#include "minecraft/auth/steps/MSAStep.h"
#include "minecraft/auth/steps/MinecraftProfileStep.h"
#include "minecraft/auth/steps/XboxAuthorizationStep.h"
#include "minecraft/auth/steps/XboxProfileStep.h"
#include "minecraft/auth/steps/XboxUserStep.h"

MSASilent::MSASilent(AccountData* data, QObject* parent) : AuthFlow(data, parent)
{
    m_steps.append(makeShared<MSAStep>(m_data, MSAStep::Action::Refresh));
    m_steps.append(makeShared<XboxUserStep>(m_data));
    m_steps.append(makeShared<XboxAuthorizationStep>(m_data, &m_data->xboxApiToken, "http://xboxlive.com", "Xbox"));
    m_steps.append(makeShared<XboxAuthorizationStep>(m_data, &m_data->mojangservicesToken, "rp://api.minecraftservices.com/", "Mojang"));
    m_steps.append(makeShared<LauncherLoginStep>(m_data));
    m_steps.append(makeShared<XboxProfileStep>(m_data));
    m_steps.append(makeShared<EntitlementsStep>(m_data));
    m_steps.append(makeShared<MinecraftProfileStep>(m_data));
    m_steps.append(makeShared<GetSkinStep>(m_data));
}

MSAInteractive::MSAInteractive(AccountData* data, QObject* parent) : AuthFlow(data, parent)
{
    m_steps.append(makeShared<MSAStep>(m_data, MSAStep::Action::Login));
    m_steps.append(makeShared<XboxUserStep>(m_data));
    m_steps.append(makeShared<XboxAuthorizationStep>(m_data, &m_data->xboxApiToken, "http://xboxlive.com", "Xbox"));
    m_steps.append(makeShared<XboxAuthorizationStep>(m_data, &m_data->mojangservicesToken, "rp://api.minecraftservices.com/", "Mojang"));
    m_steps.append(makeShared<LauncherLoginStep>(m_data));
    m_steps.append(makeShared<XboxProfileStep>(m_data));
    m_steps.append(makeShared<EntitlementsStep>(m_data));
    m_steps.append(makeShared<MinecraftProfileStep>(m_data));
    m_steps.append(makeShared<GetSkinStep>(m_data));
}
