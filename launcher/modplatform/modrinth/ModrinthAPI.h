// SPDX-FileCopyrightText: 2022-2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <qurl.h>
#include "BuildConfig.h"
#include "Result.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

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

    static std::pair<Task::Ptr, QByteArray*> currentVersion(const QString& hash, const QString& hashFormat);

    static std::pair<Task::Ptr, QByteArray*> currentVersions(const QStringList& hashes, const QString& hashFormat);

    std::pair<Task::Ptr, QByteArray*> latestVersion(
        const QString& hash,
        const QString& hashFormat,
        std::optional<std::vector<Version>> mcVersions,
        std::optional<ModPlatform::ModLoaderTypes> loaders,
        std::optional<std::vector<ModPlatform::IndexedVersionType>> releaseTypes = std::nullopt) const;

    std::pair<Task::Ptr, QByteArray*> latestVersions(
        const QStringList& hashes,
        const QString& hashFormat,
        std::optional<std::vector<Version>> mcVersions,
        std::optional<ModPlatform::ModLoaderTypes> loaders,
        std::optional<std::vector<ModPlatform::IndexedVersionType>> releaseTypes = std::nullopt) const;

   private:
    static auto getMultipleModInfoURL(const QStringList& ids) -> QString
    {
        return BuildConfig.MODRINTH_PROD_URL + QString("/projects?ids=[\"%1\"]").arg(ids.join("\",\""));
    };

    auto getVersionsURL(const VersionSearchArgs& args) const -> std::optional<QString> override;

    std::optional<QString> getDependencyURL(const DependencySearchArgs& args) const override;

    Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType /*unused*/) const override
    {
        return Modrinth::loadIndexedPackVersion(obj);
    };

   public:
    static bool validateModLoaders(ModPlatform::ModLoaderTypes loaders);

    auto getSortingMethods() const -> QList<ResourceAPI::SortingMethod> override;

    static QString getModpackIdFromUrl(const QUrl& url);

   public slots:
    Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> getProjects(const QStringList& addonIds) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const override;
    Net::RPC::Spec<QList<ModPlatform::Category>> getCategories(ModPlatform::ResourceType type) const override;

   private:
    static QUrl searchProjectsURL(const SearchArgs& args);
};
