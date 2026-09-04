// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameResourceModels.h"

#include "minecraft/PackProfile.h"
#include "modplatform/flame/FlameAPI.h"
#include "ui/pages/modplatform/flame/FlameResourcePages.h"

namespace {
bool isOptedOut(const ModPlatform::IndexedVersion& ver)
{
    return ver.downloadUrl.isEmpty();
}
}  // namespace
namespace ResourceDownload {

FlameTexturePackModel::FlameTexturePackModel(const MinecraftInstance& base)
    : TexturePackResourceModel(base, &FlameAPI::get(), Flame::debugName(), Flame::metaEntryBase())
{}

ResourceAPI::SearchArgs FlameTexturePackModel::createSearchArguments()
{
    auto args = TexturePackResourceModel::createSearchArguments();

    auto* profile = m_baseInstance.getPackProfile();
    QString instanceMinecraftVersion = profile->getComponentVersion("net.minecraft");

    // Bypass the texture pack logic, because we can't do multiple versions in the API query
    args.versions = { instanceMinecraftVersion };

    return args;
}

ResourceAPI::VersionSearchArgs FlameTexturePackModel::createVersionsArguments(const QModelIndex& entry)
{
    auto args = TexturePackResourceModel::createVersionsArguments(entry);

    // Bypass the texture pack logic, because we can't do multiple versions in the API query
    args.mcVersions = {};

    return args;
}

bool FlameTexturePackModel::optedOut(const ModPlatform::IndexedVersion& ver) const
{
    return isOptedOut(ver);
}

}  // namespace ResourceDownload
