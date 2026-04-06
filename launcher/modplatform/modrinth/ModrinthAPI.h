// SPDX-FileCopyrightText: 2022-2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "BuildConfig.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

#include <QDebug>
#include <utility>

class ModrinthAPI : public ResourceAPI {
   public:
    std::pair<Task::Ptr, QByteArray*> currentVersion(const QString& hash, const QString& hash_format) const;

    std::pair<Task::Ptr, QByteArray*> currentVersions(const QStringList& hashes, QString hash_format) const;

    std::pair<Task::Ptr, QByteArray*> latestVersion(const QString& hash,
                                                    const QString& hash_format,
                                                    std::optional<std::vector<Version>> mcVersions,
                                                    std::optional<ModPlatform::ModLoaderTypes> loaders) const;

    std::pair<Task::Ptr, QByteArray*> latestVersions(const QStringList& hashes,
                                                     const QString& hash_format,
                                                     std::optional<std::vector<Version>> mcVersions,
                                                     std::optional<ModPlatform::ModLoaderTypes> loaders) const;

    std::pair<Task::Ptr, QByteArray*> getProjects(QStringList addonIds) const override;

    static std::pair<Task::Ptr, QByteArray*> getModCategories();
    static QList<ModPlatform::Category> loadCategories(const QByteArray& response, const QString& projectType);
    static QList<ModPlatform::Category> loadModCategories(const QByteArray& response);

   public:
    auto getSortingMethods() const -> QList<ResourceAPI::SortingMethod> override;

    static auto getAuthorURL(const QString& name) -> QString { return "https://modrinth.com/user/" + name; };

    static auto getModLoaderStrings(const ModPlatform::ModLoaderTypes types) -> QStringList
    {
        QStringList l;
        for (auto loader : { ModPlatform::NeoForge, ModPlatform::Forge, ModPlatform::Fabric, ModPlatform::Quilt, ModPlatform::LiteLoader,
                             ModPlatform::DataPack, ModPlatform::Babric, ModPlatform::BTA, ModPlatform::LegacyFabric, ModPlatform::Ornithe,
                             ModPlatform::Rift }) {
            if ((types & loader) != 0U) {
                l << getModLoaderAsString(loader);
            }
        }
        return l;
    }

    static auto getModLoaderFilters(ModPlatform::ModLoaderTypes types) -> QString
    {
        QStringList l;
        for (const auto& loader : getModLoaderStrings(types)) {
            l << QString("\"categories:%1\"").arg(loader);
        }
        return l.join(',');
    }

    static auto getCategoriesFilters(const QStringList& categories) -> QString
    {
        QStringList l;
        for (const auto& cat : categories) {
            l << QString("\"categories:%1\"").arg(cat);
        }
        return l.join(',');
    }

    static QString getSideFilters(ModPlatform::Side side)
    {
        switch (side) {
            case ModPlatform::Side::ClientSide:
                return { R"("client_side:required","client_side:optional"],["server_side:optional","server_side:unsupported")" };
            case ModPlatform::Side::ServerSide:
                return { R"("server_side:required","server_side:optional"],["client_side:optional","client_side:unsupported")" };
            case ModPlatform::Side::UniversalSide:
                return { R"("client_side:required"],["server_side:required")" };
            case ModPlatform::Side::NoSide:
            // fallthrough
            default:
                return {};
        }
    }

    static QString mapMCVersionFromModrinth(QString v)
    {
        static const QString s_preString = " Pre-Release ";
        bool pre = false;
        if (v.contains("-pre")) {
            pre = true;
            v.replace("-pre", s_preString);
        }
        v.replace("-", " ");
        if (pre) {
            v.replace(" Pre Release ", s_preString);
        }
        return v;
    }

   private:
    static QString resourceTypeParameter(ModPlatform::ResourceType type)
    {
        switch (type) {
            case ModPlatform::ResourceType::Mod:
                return "mod";
            case ModPlatform::ResourceType::ResourcePack:
                return "resourcepack";
            case ModPlatform::ResourceType::ShaderPack:
                return "shader";
            case ModPlatform::ResourceType::DataPack:
                return "datapack";
            case ModPlatform::ResourceType::Modpack:
                return "modpack";
            default:
                qWarning() << "Invalid resource type for Modrinth API!";
                break;
        }

        return "";
    }

    QString createFacets(const SearchArgs& args) const
    {
        QStringList facets_list;

        if (args.loaders.has_value() && args.loaders.value() != 0) {
            facets_list.append(QString("[%1]").arg(getModLoaderFilters(args.loaders.value())));
        }
        if (args.versions.has_value() && !args.versions.value().empty()) {
            facets_list.append(QString("[%1]").arg(getGameVersionsArray(args.versions.value())));
        }
        if (args.side.has_value()) {
            auto side = getSideFilters(args.side.value());
            if (!side.isEmpty()) {
                facets_list.append(QString("[%1]").arg(side));
            }
        }
        if (args.categoryIds.has_value() && !args.categoryIds->empty()) {
            facets_list.append(QString("[%1]").arg(getCategoriesFilters(args.categoryIds.value())));
        }
        if (args.openSource) {
            facets_list.append("[\"open_source:true\"]");
        }

        facets_list.append(QString("[\"project_type:%1\"]").arg(resourceTypeParameter(args.type)));

        return QString("[%1]").arg(facets_list.join(','));
    }

   public:
    auto getSearchURL(const SearchArgs& args) const -> std::optional<QString> override
    {
        if (args.loaders.has_value() && args.loaders.value() != 0) {
            if (!validateModLoaders(args.loaders.value())) {
                qWarning() << "Modrinth - or our interface - does not support any the provided mod loaders!";
                return {};
            }
        }

        QStringList get_arguments;
        get_arguments.append(QString("offset=%1").arg(args.offset));
        get_arguments.append(QString("limit=25"));
        if (args.search.has_value()) {
            get_arguments.append(QString("query=%1").arg(args.search.value()));
        }
        if (args.sorting.has_value()) {
            get_arguments.append(QString("index=%1").arg(args.sorting.value().name));
        }
        get_arguments.append(QString("facets=%1").arg(createFacets(args)));

        return BuildConfig.MODRINTH_PROD_URL + "/search?" + get_arguments.join('&');
    };

    auto getInfoURL(const QString& id) const -> std::optional<QString> override
    {
        return BuildConfig.MODRINTH_PROD_URL + "/project/" + id;
    };

    auto getMultipleModInfoURL(const QStringList& ids) const -> QString
    {
        return BuildConfig.MODRINTH_PROD_URL + QString("/projects?ids=[\"%1\"]").arg(ids.join("\",\""));
    };

    auto getVersionsURL(const VersionSearchArgs& args) const -> std::optional<QString> override
    {
        QStringList get_arguments;
        if (args.mcVersions.has_value()) {
            get_arguments.append(QString("game_versions=[%1]").arg(getGameVersionsString(args.mcVersions.value())));
        }
        if (args.loaders.has_value()) {
            get_arguments.append(QString("loaders=[\"%1\"]").arg(getModLoaderStrings(args.loaders.value()).join("\",\"")));
        }
        get_arguments.append(QString("include_changelog=%1").arg(args.includeChangelog ? "true" : "false"));

        return QString("%1/project/%2/version%3%4")
            .arg(BuildConfig.MODRINTH_PROD_URL, args.pack->addonId.toString(), get_arguments.isEmpty() ? "" : "?", get_arguments.join('&'));
    };

    QString getGameVersionsArray(const std::vector<Version>& mcVersions) const
    {
        QString s;
        for (const auto& ver : mcVersions) {
            s += QString(R"("versions:%1",)").arg(mapMCVersionToModrinth(ver));
        }
        s.remove(s.length() - 1, 1);  // remove last comma
        return s.isEmpty() ? QString() : s;
    }

    static auto validateModLoaders(ModPlatform::ModLoaderTypes loaders) -> bool
    {
        return (loaders & (ModPlatform::NeoForge | ModPlatform::Forge | ModPlatform::Fabric | ModPlatform::Quilt | ModPlatform::LiteLoader |
                           ModPlatform::DataPack | ModPlatform::Babric | ModPlatform::BTA | ModPlatform::LegacyFabric |
                           ModPlatform::Ornithe | ModPlatform::Rift)) != 0;
    }

    std::optional<QString> getDependencyURL(const DependencySearchArgs& args) const override
    {
        return args.dependency.version.length() != 0
                   ? QString("%1/version/%2").arg(BuildConfig.MODRINTH_PROD_URL, args.dependency.version)
                   : QString(R"(%1/project/%2/version?game_versions=["%3"]&loaders=["%4"]&include_changelog=%5)")
                         .arg(BuildConfig.MODRINTH_PROD_URL)
                         .arg(args.dependency.addonId.toString())
                         .arg(mapMCVersionToModrinth(args.mcVersion))
                         .arg(getModLoaderStrings(args.loader).join("\",\""))
                         .arg(args.includeChangelog ? "true" : "false");
    };

    QJsonArray documentToArray(QJsonDocument& obj) const override { return obj.object().value("hits").toArray(); }
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) const override { Modrinth::loadIndexedPack(m, obj); }
    ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType /*unused*/) const override
    {
        return Modrinth::loadIndexedPackVersion(obj);
    };
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) const override { Modrinth::loadExtraPackData(m, obj); }
};
