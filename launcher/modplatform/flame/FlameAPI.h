// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <algorithm>
#include <memory>
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/helpers/NetworkResourceAPI.h"

class FlameAPI : public NetworkResourceAPI {
   public:
    auto getModFileChangelog(int modId, int fileId) -> QString;
    auto getModDescription(int modId) -> QString;

    auto getLatestVersion(VersionSearchArgs&& args) -> ModPlatform::IndexedVersion;

    Task::Ptr getProjects(QStringList addonIds, std::shared_ptr<QByteArray> response) const override;
    Task::Ptr matchFingerprints(const QList<uint>& fingerprints, std::shared_ptr<QByteArray> response);
    Task::Ptr getFiles(const QStringList& fileIds, std::shared_ptr<QByteArray> response) const;

    [[nodiscard]] auto getSortingMethods() const -> QList<ResourceAPI::SortingMethod> override;

   private:
    static int getClassId(ModPlatform::ResourceType type)
    {
        switch (type) {
            default:
            case ModPlatform::ResourceType::MOD:
                return 6;
            case ModPlatform::ResourceType::RESOURCE_PACK:
                return 12;
        }
    }

    static int getMappedModLoader(ModLoaderTypes loaders)
    {
        // https://docs.curseforge.com/?http#tocS_ModLoaderType
        if (loaders & Forge)
            return 1;
        if (loaders & Fabric)
            return 4;
        // TODO: remove this once Quilt drops official Fabric support
        if (loaders & Quilt)  // NOTE: Most if not all Fabric mods should work *currently*
            return 4;         // Quilt would probably be 5
        return 0;
    }

   private:
    [[nodiscard]] std::optional<QString> getSearchURL(SearchArgs const& args) const override
    {
        auto gameVersionStr =
            args.versions.has_value() ? QString("gameVersion=%1").arg(args.versions.value().front().toString()) : QString();

        QStringList get_arguments;
        get_arguments.append(QString("classId=%1").arg(getClassId(args.type)));
        get_arguments.append(QString("index=%1").arg(args.offset));
        get_arguments.append("pageSize=25");
        if (args.search.has_value())
            get_arguments.append(QString("searchFilter=%1").arg(args.search.value()));
        if (args.sorting.has_value())
            get_arguments.append(QString("sortField=%1").arg(args.sorting.value().index));
        get_arguments.append("sortOrder=desc");
        if (args.loaders.has_value())
            get_arguments.append(QString("modLoaderType=%1").arg(getMappedModLoader(args.loaders.value())));
        get_arguments.append(gameVersionStr);

        return "https://api.curseforge.com/v1/mods/search?gameId=432&" + get_arguments.join('&');
    };

    [[nodiscard]] std::optional<QString> getInfoURL(QString const& id) const override
    {
        return QString("https://api.curseforge.com/v1/mods/%1").arg(id);
    };

    [[nodiscard]] std::optional<QString> getVersionsURL(VersionSearchArgs const& args) const override
    {
        auto mappedModLoader = getMappedModLoader(args.loaders.value());
        auto addonId = args.pack.addonId.toString();
        if (args.loaders.value() & Quilt) {
            auto overide = ModPlatform::getOverrideDeps();
            auto over = std::find_if(overide.cbegin(), overide.cend(), [addonId](auto dep) {
                return dep.provider == ModPlatform::ResourceProvider::FLAME && addonId == dep.quilt;
            });
            if (over != overide.cend()) {
                mappedModLoader = 5;
            }
        }
        QString url{ QString("https://api.curseforge.com/v1/mods/%1/files?pageSize=10000&").arg(addonId) };

        QStringList get_parameters;
        if (args.mcVersions.has_value())
            get_parameters.append(QString("gameVersion=%1").arg(args.mcVersions.value().front().toString()));
        if (args.loaders.has_value())
            get_parameters.append(QString("modLoaderType=%1").arg(mappedModLoader));

        return url + get_parameters.join('&');
    };

    [[nodiscard]] std::optional<QString> getDependencyURL(DependencySearchArgs const& args) const override
    {
        auto mappedModLoader = getMappedModLoader(args.loader);
        auto addonId = args.dependency.addonId.toString();
        if (args.loader & Quilt) {
            auto overide = ModPlatform::getOverrideDeps();
            auto over = std::find_if(overide.cbegin(), overide.cend(), [addonId](auto dep) {
                return dep.provider == ModPlatform::ResourceProvider::FLAME && addonId == dep.quilt;
            });
            if (over != overide.cend()) {
                mappedModLoader = 5;
            }
        }
        return QString("https://api.curseforge.com/v1/mods/%1/files?pageSize=10000&gameVersion=%2&modLoaderType=%3")
            .arg(addonId)
            .arg(args.mcVersion.toString())
            .arg(mappedModLoader);
    };
};
