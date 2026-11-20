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

#include <QFlags>
#include <QString>
#include <QVariant>
#include <cstdint>
#include <functional>
#include <memory>

#include "modplatform/ResourceType.h"

class BaseInstance;
class BasePage;
class InstanceTask;
class NewInstanceDialog;
class ResourceAPI;

namespace ResourceDownload {
class ResourceDownloadDialog;
}

namespace ModPlatform {

/**
 * Capability flags describing what a registered mod/pack provider can do.
 *
 * UI and workflow code should query these instead of hard-coding provider IDs
 * when deciding whether to offer a page, API, or install path.
 */
enum class ProviderCapability : std::uint32_t {
    None = 0U,

    /** Contributes a page to NewInstanceDialog. */
    NewInstancePage = 1U << 0U,

    /** Offers mod downloads through ResourceDownloadDialog. */
    ResourceMods = 1U << 1U,

    /** Offers resource-pack downloads. */
    ResourcePacks = 1U << 2U,

    /** Offers texture-pack downloads. */
    TexturePacks = 1U << 3U,

    /** Offers shader-pack downloads. */
    ShaderPacks = 1U << 4U,

    /** Offers data-pack downloads. */
    DataPacks = 1U << 5U,

    /** Provides a ResourceAPI implementation. */
    HasResourceAPI = 1U << 6U,

    /** Can create an InstanceTask for installing/creating from pack data. */
    InstallTask = 1U << 7U,

    /** Requires an application-level API key / SupportsFlame-style gate. */
    RequiresApiKey = 1U << 8U,
};
Q_DECLARE_FLAGS(ProviderCapabilityFlags, ProviderCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(ProviderCapabilityFlags)

/** Creates a NewInstanceDialog page for this provider. Caller owns the page. */
using NewInstancePageFactory = std::function<BasePage*(NewInstanceDialog* dialog)>;

/** Creates a resource-download page. Caller owns the page. */
using ResourcePageFactory =
    std::function<BasePage*(ResourceDownload::ResourceDownloadDialog* dialog, BaseInstance& instance)>;

/**
 * Creates an install/creation task from provider-specific pack data.
 * Encoding of \p packData is defined by the provider. Caller owns the task.
 */
using InstallTaskFactory = std::function<InstanceTask*(const QVariant& packData)>;

/** Creates a ResourceAPI for this provider. */
using ResourceAPIFactory = std::function<std::unique_ptr<ResourceAPI>()>;

/**
 * Descriptor for a statically registered mod/pack provider.
 *
 * Factories may be empty when the corresponding capability is not set.
 * Runtime availability (API keys, loaders, etc.) is expressed via \ref isAvailable.
 */
struct ModPlatformProvider {
    /** Stable serialized identifier (e.g. "modrinth", "flame"). */
    QString id;

    /** Human-readable name shown in the UI. */
    QString displayName;

    /** Theme icon name (e.g. "modrinth", "flame"). */
    QString iconName;

    /**
     * HttpMetaCache base key used for this provider's downloads
     * (e.g. "ModrinthPacks", "FlameMods").
     */
    QString cacheKey;

    ProviderCapabilityFlags capabilities = ProviderCapability::None;

    NewInstancePageFactory createNewInstancePage;
    ResourcePageFactory createModPage;
    ResourcePageFactory createResourcePackPage;
    ResourcePageFactory createTexturePackPage;
    ResourcePageFactory createShaderPackPage;
    ResourcePageFactory createDataPackPage;
    InstallTaskFactory createInstallTask;
    ResourceAPIFactory createResourceAPI;

    /**
     * Optional runtime gate. When unset, the provider is treated as available.
     * Use for SupportsFlame / loader checks that are not compile-time.
     */
    std::function<bool()> isAvailable;

    bool hasCapability(ProviderCapability capability) const { return capabilities.testFlag(capability); }

    bool available() const { return !isAvailable || isAvailable(); }

    /** Resource-download page factory for \p type, or an empty function. */
    ResourcePageFactory resourcePageFactory(ResourceType type) const
    {
        switch (type) {
            case ResourceType::Mod:
                return createModPage;
            case ResourceType::ResourcePack:
                return createResourcePackPage;
            case ResourceType::TexturePack:
                return createTexturePackPage;
            case ResourceType::ShaderPack:
                return createShaderPackPage;
            case ResourceType::DataPack:
                return createDataPackPage;
            default:
                return {};
        }
    }
};

/**
 * Convenience: map a resource type to the matching provider capability flag.
 */
inline ProviderCapability capabilityForResourceType(ResourceType type)
{
    switch (type) {
        case ResourceType::Mod:
            return ProviderCapability::ResourceMods;
        case ResourceType::ResourcePack:
            return ProviderCapability::ResourcePacks;
        case ResourceType::TexturePack:
            return ProviderCapability::TexturePacks;
        case ResourceType::ShaderPack:
            return ProviderCapability::ShaderPacks;
        case ResourceType::DataPack:
            return ProviderCapability::DataPacks;
        case ResourceType::Modpack:
            return ProviderCapability::NewInstancePage;
        default:
            return ProviderCapability::None;
    }
}

}  // namespace ModPlatform

// Static registration: see REGISTER_MOD_PLATFORM in modplatform/ModPlatformRegistry.h