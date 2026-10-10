// SPDX-FileCopyrightText: 2022-2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <utility>

class ModrinthAPI final : public ResourceAPI {
   public:
    static const ModrinthAPI& get()
    {
        static const ModrinthAPI s_instance;
        return s_instance;
    }

   public:
    static bool validateModLoaders(ModPlatform::ModLoaderTypes loaders);

    auto getSortingMethods() const -> QList<ResourceAPI::SortingMethod> override;

   public slots:
    Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> getProjects(const QStringList& addonIds) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const override;
    Net::RPC::Spec<QList<ModPlatform::Category>> getCategories(ModPlatform::ResourceType type) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const VersionSearchArgs& args) const override;
    Net::RPC::Spec<ModPlatform::IndexedVersion> getVersion(const QString& id, const QString& versionId) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const QStringList& versionIds) const override;

    static Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> latestVersions(
        const QStringList& hashes,
        const QString& hashFormat,
        std::optional<std::vector<Version>> mcVersions,
        std::optional<ModPlatform::ModLoaderTypes> loaders,
        const std::optional<std::vector<ModPlatform::IndexedVersionType>>& releaseTypes = std::nullopt);
    static std::pair<NetJob::Ptr, QHash<QString, ModPlatform::IndexedVersion>*> latestVersionsTask(
        const QStringList& hashes,
        const QString& hashFormat,
        std::optional<std::vector<Version>> mcVersions,
        std::optional<ModPlatform::ModLoaderTypes> loaders,
        const std::optional<std::vector<ModPlatform::IndexedVersionType>>& releaseTypes = std::nullopt);

    static Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> currentVersions(const QStringList& hashes,
                                                                                       const QString& hashFormat);
    static std::pair<NetJob::Ptr, QHash<QString, ModPlatform::IndexedVersion>*> currentVersionsTask(const QStringList& hashes,
                                                                                                    const QString& hashFormat);

   private:
    static QUrl searchProjectsURL(const SearchArgs& args);
    static QUrl getVersionsURL(const VersionSearchArgs& args);
};
