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

#pragma once

#include <QList>
#include <QString>

#include <memory>
#include <vector>

#include "modplatform/ModPlatformProvider.h"

namespace ModPlatform {

/**
 * Process-wide static registry of mod/pack providers.
 *
 * Registration is compile-time / static-init only for this phase.
 * Dynamic plugin loading is intentionally not supported.
 *
 * Iteration order matches registration order so UI page order stays
 * deterministic when dialogs switch to registry-driven construction.
 */
class ModPlatformRegistry {
   public:
    static ModPlatformRegistry& instance();

    ModPlatformRegistry(const ModPlatformRegistry&) = delete;
    ModPlatformRegistry& operator=(const ModPlatformRegistry&) = delete;

    /**
     * Register a provider descriptor.
     * \return false if \p provider.id is empty or already registered.
     */
    bool registerProvider(ModPlatformProvider provider);

    /** Lookup by stable id, or nullptr. */
    const ModPlatformProvider* find(const QString& id) const;

    /** All registered providers, in registration order. */
    QList<const ModPlatformProvider*> providers() const;

    /**
     * Providers that advertise every flag in \p required and pass \ref ModPlatformProvider::available.
     */
    QList<const ModPlatformProvider*> providersWith(ProviderCapabilityFlags required) const;

    /** Drop all registrations. Intended for unit tests only. */
    void resetForTesting();

   private:
    ModPlatformRegistry() = default;

    // unique_ptr keeps returned const ModPlatformProvider* stable across appends.
    std::vector<std::unique_ptr<ModPlatformProvider>> m_providers;
};

}  // namespace ModPlatform

/**
 * Statically register a mod/pack provider.
 *
 * \param UniqueName  Unique token used only to form a static symbol (e.g. modrinth).
 * \param ProviderExpression  Expression yielding a ModPlatform::ModPlatformProvider
 *                            (typically a lambda call that builds the descriptor).
 *
 * Example:
 * \code
 * #include "modplatform/ModPlatformRegistry.h"
 *
 * REGISTER_MOD_PLATFORM(modrinth, [] {
 *     ModPlatform::ModPlatformProvider provider;
 *     provider.id = QStringLiteral("modrinth");
 *     provider.displayName = QStringLiteral("Modrinth");
 *     provider.iconName = QStringLiteral("modrinth");
 *     provider.cacheKey = QStringLiteral("ModrinthPacks");
 *     provider.capabilities = ModPlatform::ProviderCapability::NewInstancePage |
 *                             ModPlatform::ProviderCapability::ResourceMods |
 *                             ModPlatform::ProviderCapability::HasResourceAPI;
 *     provider.createResourceAPI = [] { return std::make_unique<ModrinthAPI>(); };
 *     return provider;
 * }());
 * \endcode
 *
 * Place the macro at namespace scope in one translation unit belonging to the
 * provider. Do not register the same id from multiple TUs.
 */
#define REGISTER_MOD_PLATFORM(UniqueName, ProviderExpression) \
    static const bool registered_mod_platform_##UniqueName =  \
        ::ModPlatform::ModPlatformRegistry::instance().registerProvider(ProviderExpression)
