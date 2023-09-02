// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameResourceModels.h"

#include "Json.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"

namespace ResourceDownload {

FlameModModel::FlameModModel(BaseInstance& base) : ModModel(base, new FlameAPI) {}

void FlameModModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

// We already deal with the URLs when initializing the pack, due to the API response's structure
void FlameModModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadBody(m, obj);
}

void FlameModModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto FlameModModel::loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr) -> ModPlatform::IndexedVersion
{
    return FlameMod::loadDependencyVersions(m, arr, &m_base_instance);
}

auto FlameModModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

FlameResourcePackModel::FlameResourcePackModel(const BaseInstance& base) : ResourcePackResourceModel(base, new FlameAPI) {}

void FlameResourcePackModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

// We already deal with the URLs when initializing the pack, due to the API response's structure
void FlameResourcePackModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadBody(m, obj);
}

void FlameResourcePackModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto FlameResourcePackModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

FlameTexturePackModel::FlameTexturePackModel(const BaseInstance& base) : TexturePackResourceModel(base, new FlameAPI) {}

void FlameTexturePackModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

// We already deal with the URLs when initializing the pack, due to the API response's structure
void FlameTexturePackModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadBody(m, obj);
}

void FlameTexturePackModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);

    QVector<ModPlatform::IndexedVersion> filtered_versions(m.versions.size());

    // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
    for (auto const& version : m.versions) {
        auto const& mc_versions = version.mcVersion;

        if (std::any_of(mc_versions.constBegin(), mc_versions.constEnd(),
                        [this](auto const& mc_version) { return Version(mc_version) <= maximumTexturePackVersion(); }))
            filtered_versions.push_back(version);
    }

    m.versions = filtered_versions;
}

ResourceAPI::SearchArgs FlameTexturePackModel::createSearchArguments()
{
    auto args = TexturePackResourceModel::createSearchArguments();

    auto profile = static_cast<const MinecraftInstance&>(m_base_instance).getPackProfile();
    QString instance_minecraft_version = profile->getComponentVersion("net.minecraft");

    // Bypass the texture pack logic, because we can't do multiple versions in the API query
    args.versions = { instance_minecraft_version };

    return args;
}

ResourceAPI::VersionSearchArgs FlameTexturePackModel::createVersionsArguments(QModelIndex& entry)
{
    auto args = TexturePackResourceModel::createVersionsArguments(entry);

    // Bypass the texture pack logic, because we can't do multiple versions in the API query
    args.mcVersions = {};

    return args;
}

auto FlameTexturePackModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

}  // namespace ResourceDownload
