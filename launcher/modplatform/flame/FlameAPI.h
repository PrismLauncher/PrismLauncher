// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <memory>
#include "Json.h"
#include "Version.h"
#include "api/structures/ModLoader.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameModIndex.h"

class FlameAPI : public ResourceAPI {
   public:
    QString getModFileChangelog(int modId, int fileId);
    QString getModDescription(int modId);

    std::optional<ModPlatform::IndexedVersion> getLatestVersion(QList<ModPlatform::IndexedVersion> versions,
                                                                QList<Platform::ModLoader> instanceLoaders,
                                                                Platform::ModLoaders fallback);

    Task::Ptr getProjects(QStringList addonIds, std::shared_ptr<QByteArray> response) const override;
    Task::Ptr matchFingerprints(const QList<uint>& fingerprints, std::shared_ptr<QByteArray> response);
    Task::Ptr getFiles(const QStringList& fileIds, std::shared_ptr<QByteArray> response) const;
    Task::Ptr getFile(const QString& addonId, const QString& fileId, std::shared_ptr<QByteArray> response) const;

    static Task::Ptr getCategories(std::shared_ptr<QByteArray> response, Platform::ResourceType type);
    static Task::Ptr getModCategories(std::shared_ptr<QByteArray> response);
    static QList<ModPlatform::Category> loadModCategories(std::shared_ptr<QByteArray> response);

    [[nodiscard]] QList<ResourceAPI::SortingMethod> getSortingMethods() const override;

    static inline bool validateModLoaders(Platform::ModLoaders loaders)
    {
        return loaders &
               (Platform::ModLoader::NeoForge | Platform::ModLoader::Forge | Platform::ModLoader::Fabric | Platform::ModLoader::Quilt);
    }

   private:
    static int getClassId(Platform::ResourceType type)
    {
        switch (type) {
            default:
            case Platform::ResourceType::Mod:
                return 6;
            case Platform::ResourceType::ResourcePack:
                return 12;
            case Platform::ResourceType::ShaderPack:
                return 6552;
            case Platform::ResourceType::Modpack:
                return 4471;
        }
    }

    static int getMappedModLoader(Platform::ModLoader loaders)
    {
        // https://docs.curseforge.com/?http#tocS_ModLoaderType
        switch (loaders) {
            case Platform::ModLoader::Forge:
                return 1;
            case Platform::ModLoader::Cauldron:
                return 2;
            case Platform::ModLoader::LiteLoader:
                return 3;
            case Platform::ModLoader::Fabric:
                return 4;
            case Platform::ModLoader::Quilt:
                return 5;
            case Platform::ModLoader::NeoForge:
                return 6;
        }
        return 0;
    }

    static const QStringList getModLoaderStrings(const Platform::ModLoaders types)
    {
        QStringList l;
        for (auto loader :
             { Platform::ModLoader::NeoForge, Platform::ModLoader::Forge, Platform::ModLoader::Fabric, Platform::ModLoader::Quilt }) {
            if (types & loader) {
                l << QString::number(getMappedModLoader(loader));
            }
        }
        return l;
    }

    static const QString getModLoaderFilters(Platform::ModLoaders types) { return "[" + getModLoaderStrings(types).join(',') + "]"; }

   public:
    [[nodiscard]] std::optional<QString> getSearchURL(SearchArgs const& args) const override
    {
        QStringList get_arguments;
        get_arguments.append(QString("classId=%1").arg(getClassId(args.type)));
        get_arguments.append(QString("index=%1").arg(args.offset));
        get_arguments.append("pageSize=25");
        if (args.search.has_value())
            get_arguments.append(QString("searchFilter=%1").arg(args.search.value()));
        if (args.sorting.has_value())
            get_arguments.append(QString("sortField=%1").arg(args.sorting.value().index));
        get_arguments.append("sortOrder=desc");
        if (args.loaders.has_value() && args.loaders.value() != 0)
            get_arguments.append(QString("modLoaderTypes=%1").arg(getModLoaderFilters(args.loaders.value())));
        if (args.categoryIds.has_value() && !args.categoryIds->empty())
            get_arguments.append(QString("categoryIds=[%1]").arg(args.categoryIds->join(",")));

        if (args.versions.has_value() && !args.versions.value().empty())
            get_arguments.append(QString("gameVersion=%1").arg(args.versions.value().front().toString()));

        return "https://api.curseforge.com/v1/mods/search?gameId=432&" + get_arguments.join('&');
    }

    [[nodiscard]] std::optional<QString> getVersionsURL(VersionSearchArgs const& args) const override
    {
        auto addonId = args.pack.addonId.toString();
        QString url = QString("https://api.curseforge.com/v1/mods/%1/files?pageSize=10000").arg(addonId);

        if (args.mcVersions.has_value())
            url += QString("&gameVersion=%1").arg(args.mcVersions.value().front().toString());

        if (args.loaders.has_value() && Platform::ModloaderUtils::hasSingleSelected(args.loaders.value())) {
            int mappedModLoader = getMappedModLoader(static_cast<Platform::ModLoader>(static_cast<int>(args.loaders.value())));
            url += QString("&modLoaderType=%1").arg(mappedModLoader);
        }
        return url;
    }

    QJsonArray documentToArray(QJsonDocument& obj) const override { return Json::ensureArray(obj.object(), "data"); }
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) const override { FlameMod::loadIndexedPack(m, obj); }
    ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, Platform::ResourceType resourceType) const override
    {
        auto arr = FlameMod::loadIndexedPackVersion(obj);
        if (resourceType != Platform::ResourceType::TexturePack) {
            return arr;
        }
        // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
        auto const& mc_versions = arr.mcVersion;

        if (std::any_of(mc_versions.constBegin(), mc_versions.constEnd(),
                        [](auto const& mc_version) { return Version(mc_version) <= Version("1.6"); })) {
            return arr;
        }
        return {};
    };
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, [[maybe_unused]] QJsonObject&) const override { FlameMod::loadBody(m); }

   private:
    [[nodiscard]] std::optional<QString> getInfoURL(QString const& id) const override
    {
        return QString("https://api.curseforge.com/v1/mods/%1").arg(id);
    }
    [[nodiscard]] std::optional<QString> getDependencyURL(DependencySearchArgs const& args) const override
    {
        auto addonId = args.dependency.addonId.toString();
        auto url =
            QString("https://api.curseforge.com/v1/mods/%1/files?pageSize=10000&gameVersion=%2").arg(addonId, args.mcVersion.toString());
        if (args.loader && Platform::ModloaderUtils::hasSingleSelected(args.loader)) {
            int mappedModLoader = getMappedModLoader(static_cast<Platform::ModLoader>(static_cast<int>(args.loader)));
            url += QString("&modLoaderType=%1").arg(mappedModLoader);
        }
        return url;
    }
};
