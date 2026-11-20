// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "modplatform/ModPlatformRegistry.h"

#include <QDebug>

namespace ModPlatform {

ModPlatformRegistry& ModPlatformRegistry::instance()
{
    static ModPlatformRegistry registry;
    return registry;
}

bool ModPlatformRegistry::registerProvider(ModPlatformProvider provider)
{
    if (provider.id.isEmpty()) {
        qWarning() << "ModPlatformRegistry: refusing to register provider with empty id";
        return false;
    }

    if (find(provider.id) != nullptr) {
        qWarning() << "ModPlatformRegistry: provider already registered:" << provider.id;
        return false;
    }

    m_providers.push_back(std::make_unique<ModPlatformProvider>(std::move(provider)));
    return true;
}

const ModPlatformProvider* ModPlatformRegistry::find(const QString& id) const
{
    for (const auto& provider : m_providers) {
        if (provider->id == id) {
            return provider.get();
        }
    }
    return nullptr;
}

QList<const ModPlatformProvider*> ModPlatformRegistry::providers() const
{
    QList<const ModPlatformProvider*> result;
    result.reserve(static_cast<qsizetype>(m_providers.size()));
    for (const auto& provider : m_providers) {
        result.append(provider.get());
    }
    return result;
}

QList<const ModPlatformProvider*> ModPlatformRegistry::providersWith(ProviderCapabilityFlags required) const
{
    QList<const ModPlatformProvider*> result;
    for (const auto& provider : m_providers) {
        if ((provider->capabilities & required) != required) {
            continue;
        }
        if (!provider->available()) {
            continue;
        }
        result.append(provider.get());
    }
    return result;
}

void ModPlatformRegistry::resetForTesting()
{
    m_providers.clear();
}

}  // namespace ModPlatform
