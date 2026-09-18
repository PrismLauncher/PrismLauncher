// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

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

    static std::pair<Task::Ptr, QByteArray*> matchFingerprints(const QList<uint>& fingerprints);
    static std::pair<Task::Ptr, QByteArray*> getFiles(const QStringList& fileIds);
    static std::pair<Task::Ptr, QByteArray*> getFile(const QString& addonId, const QString& fileId);

    static ModPlatform::ResourceType getResourceType(int classId);

   public:
    static bool validateModLoaders(ModPlatform::ModLoaderTypes loaders);

    QList<ResourceAPI::SortingMethod> getSortingMethods() const override;

   public slots:
    Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> getProjects(const QStringList& addonIds) const override;
    std::optional<Net::RPC::Spec<bool>> getProjectExtra(ModPlatform::IndexedPack& pack) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const override;
    Net::RPC::Spec<QList<ModPlatform::Category>> getCategories(ModPlatform::ResourceType type) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const VersionSearchArgs& args) const override;

   private:
    static QUrl searchProjectsURL(const SearchArgs& args);
    static QUrl getVersionsURL(const VersionSearchArgs& args);
};
