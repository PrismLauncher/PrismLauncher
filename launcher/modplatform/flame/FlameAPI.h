// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <cstdint>
#include "BuildConfig.h"
#include "Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameModIndex.h"

class FlameAPI final : public ResourceAPI {
   public:
    static const FlameAPI& get()
    {
        static const FlameAPI s_instance;
        return s_instance;
    }

    static QString getModFileChangelog(int modId, int fileId);
    static QString getModDescription(int modId);

    static std::optional<ModPlatform::IndexedVersion> getLatestVersion(const QList<ModPlatform::IndexedVersion>& versions,
                                                                       const QList<ModPlatform::ModLoaderType>& instanceLoaders,
                                                                       ModPlatform::ModLoaderTypes modLoaders,
                                                                       bool checkLoaders);

    Net::Spec<QList<ModPlatform::IndexedPack::Ptr>> getProjects(QStringList addonIds) const override;
    static Net::Spec<QList<FlameMod::FingerprintMatch>> matchFingerprints(const QList<uint>& fingerprints);
    static Net::Spec<QList<ModPlatform::IndexedVersion>> getFiles(const QStringList& fileIds);
    static Net::Spec<ModPlatform::IndexedVersion> getFile(const QString& addonId, const QString& fileId);

    Net::Spec<QList<ModPlatform::Category>> getCategories(ModPlatform::ResourceType type) const override;

    QList<ResourceAPI::SortingMethod> getSortingMethods() const override;

    static bool validateModLoaders(ModPlatform::ModLoaderTypes loaders)
    {
        return loaders.testAnyFlag(ModPlatform::NeoForge | ModPlatform::Forge | ModPlatform::Fabric | ModPlatform::Quilt);
    }

   private:
    // NOTE: prevent creation and deletion of type - get should be used instead
    FlameAPI() = default;

    ~FlameAPI() = default;

    static int getClassId(ModPlatform::ResourceType type)
    {
        switch (type) {
            default:
            case ModPlatform::ResourceType::Mod:
                return 6;
            case ModPlatform::ResourceType::ResourcePack:
                return 12;
            case ModPlatform::ResourceType::ShaderPack:
                return 6552;
            case ModPlatform::ResourceType::Modpack:
                return 4471;
            case ModPlatform::ResourceType::DataPack:
                return 6945;
        }
    }

    static int getMappedModLoader(ModPlatform::ModLoaderType loaders)
    {
        // https://docs.curseforge.com/?http#tocS_ModLoaderType
        switch (loaders) {
            case ModPlatform::Forge:
                return 1;
            case ModPlatform::Cauldron:
                return 2;
            case ModPlatform::LiteLoader:
                return 3;
            case ModPlatform::Fabric:
                return 4;
            case ModPlatform::Quilt:
                return 5;
            case ModPlatform::NeoForge:
                return 6;
            case ModPlatform::DataPack:
            case ModPlatform::Babric:
            case ModPlatform::BTA:
            case ModPlatform::LegacyFabric:
            case ModPlatform::Ornithe:
            case ModPlatform::Rift:
            case ModPlatform::None:
                break;  // not supported
        }
        return 0;
    }

    static QStringList getModLoaderStrings(const ModPlatform::ModLoaderTypes types)
    {
        QStringList l;
        for (auto loader : { ModPlatform::NeoForge, ModPlatform::Forge, ModPlatform::Fabric, ModPlatform::Quilt }) {
            if (types.testFlag(loader)) {
                l << QString::number(getMappedModLoader(loader));
            }
        }
        return l;
    }

    static QString getModLoaderFilters(ModPlatform::ModLoaderTypes types) { return "[" + getModLoaderStrings(types).join(',') + "]"; }

   public:
    std::optional<QString> getSearchURL(const SearchArgs& args) const override
    {
        QStringList getArguments;
        getArguments.append(QString("classId=%1").arg(getClassId(args.type)));
        getArguments.append(QString("index=%1").arg(args.offset));
        getArguments.append("pageSize=25");
        if (args.search.has_value()) {
            getArguments.append(QString("searchFilter=%1").arg(args.search.value()));
        }
        if (args.sorting.has_value()) {
            getArguments.append(QString("sortField=%1").arg(args.sorting.value().index));
        }
        getArguments.append("sortOrder=desc");
        if (args.loaders.has_value()) {
            ModPlatform::ModLoaderTypes loaders = args.loaders.value();
            loaders &= ~static_cast<std::uint16_t>(ModPlatform::ModLoaderType::DataPack);
            if (loaders != 0) {
                getArguments.append(QString("modLoaderTypes=%1").arg(getModLoaderFilters(loaders)));
            }
        }
        if (args.categoryIds.has_value() && !args.categoryIds->empty()) {
            getArguments.append(QString("categoryIds=[%1]").arg(args.categoryIds->join(",")));
        }

        if (args.versions.has_value() && !args.versions.value().empty()) {
            getArguments.append(QString("gameVersion=%1").arg(args.versions.value().front().toString()));
        }

        return BuildConfig.FLAME_BASE_URL + "/mods/search?gameId=432&" + getArguments.join('&');
    }

    std::optional<QString> getVersionsURL(const VersionSearchArgs& args) const override
    {
        auto addonId = args.pack->addonId.toString();
        QString url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files?pageSize=10000").arg(addonId);

        if (args.mcVersions.has_value()) {
            url += QString("&gameVersion=%1").arg(args.mcVersions.value().front().toString());
        }

        if (args.loaders.has_value() && args.loaders.value() != ModPlatform::ModLoaderType::DataPack &&
            ModPlatform::hasSingleModLoaderSelected(args.loaders.value())) {
            int mappedModLoader = getMappedModLoader(static_cast<ModPlatform::ModLoaderType>(static_cast<int>(args.loaders.value())));
            url += QString("&modLoaderType=%1").arg(mappedModLoader);
        }
        return url;
    }

    QJsonArray documentToArray(QJsonDocument& obj) const override { return obj.object()["data"].toArray(); }
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) const override { FlameMod::loadIndexedPack(m, obj); }
    ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType resourceType) const override
    {
        auto arr = FlameMod::loadIndexedPackVersion(obj);
        if (resourceType != ModPlatform::ResourceType::TexturePack) {
            return arr;
        }
        // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
        const auto& mcVersions = arr.mcVersion;

        if (std::any_of(mcVersions.constBegin(), mcVersions.constEnd(),
                        [](const auto& mcVersion) { return Version(mcVersion) <= Version("1.6"); })) {
            return arr;
        }
        return {};
    };
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, [[maybe_unused]] QJsonObject& /*unused*/) const override { FlameMod::loadBody(m); }

   private:
    std::optional<QString> getInfoURL(const QString& id) const override { return QString(BuildConfig.FLAME_BASE_URL + "/mods/%1").arg(id); }
    std::optional<QString> getDependencyURL(const DependencySearchArgs& args) const override
    {
        auto addonId = args.dependency.addonId.toString();
        auto url =
            QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files?pageSize=10000&gameVersion=%2").arg(addonId, args.mcVersion.toString());
        if ((args.loader != 0U) && ModPlatform::hasSingleModLoaderSelected(args.loader)) {
            int mappedModLoader = getMappedModLoader(static_cast<ModPlatform::ModLoaderType>(static_cast<int>(args.loader)));
            url += QString("&modLoaderType=%1").arg(mappedModLoader);
        }
        return url;
    }
};
