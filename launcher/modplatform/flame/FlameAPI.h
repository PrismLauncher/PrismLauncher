// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <qstringview.h>
#include <qurl.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include "Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/flame/FlamePackIndex.h"

class FlameAPI final : public ResourceAPI {
   public:
    static const FlameAPI& get()
    {
        static const FlameAPI s_instance;
        return s_instance;
    }

    static QString getModFileChangelog(int modId, int fileId);

    static std::optional<ModPlatform::IndexedVersion> getLatestVersion(const QList<ModPlatform::IndexedVersion>& versions,
                                                                       const QList<ModPlatform::ModLoaderType>& instanceLoaders,
                                                                       ModPlatform::ModLoaderTypes fallback,
                                                                       bool checkLoaders);

    std::pair<Task::Ptr, QByteArray*> getProjects(QStringList addonIds) const override;
    static std::pair<Task::Ptr, QByteArray*> matchFingerprints(const QList<uint>& fingerprints);
    static std::pair<Task::Ptr, QByteArray*> getFiles(const QStringList& fileIds);
    static std::pair<Task::Ptr, QByteArray*> getFile(const QString& addonId, const QString& fileId);

    static std::pair<Task::Ptr, QByteArray*> getCategories(ModPlatform::ResourceType type);
    std::pair<Task::Ptr, QByteArray*> getModCategories() const override;
    QList<ModPlatform::Category> loadModCategories(const QByteArray& response) const override;

    QList<ResourceAPI::SortingMethod> getSortingMethods() const override;

    static ModPlatform::ResourceType getResourceType(int classId);

   public:
    std::optional<QString> getVersionsURL(const VersionSearchArgs& args) const override;

    Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType resourceType) const override
    {
        TRY_INTO(const auto& arr, FlameMod::loadIndexedPackVersion(obj))
        if (resourceType != ModPlatform::ResourceType::TexturePack) {
            return arr;
        }
        // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
        const auto& mcVersions = arr.mcVersion;

        if (std::any_of(mcVersions.constBegin(), mcVersions.constEnd(),
                        [](const auto& mcVersion) { return Version(mcVersion) <= Version("1.6"); })) {
            return arr;
        }
        return ModPlatform::IndexedVersion{};
    };

   public:
    static bool validateModLoaders(ModPlatform::ModLoaderTypes loaders);

   public slots:
    Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const override;
    std::optional<Net::RPC::Spec<bool>> getProjectExtra(ModPlatform::IndexedPack& pack) const override;

    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const override;

   private:
    std::optional<QString> getDependencyURL(const DependencySearchArgs& args) const override;

    static QUrl searchProjectsURL(const SearchArgs& args);
};
