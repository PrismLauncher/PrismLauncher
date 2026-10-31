// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QHash>
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
    Net::RPC::Spec<ModPlatform::IndexedVersion> getVersion(const QString& id, const QString& versionId) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const QStringList& versionIds) const override;

    static Net::RPC::Spec<QString> getChangelog(const QString& id, const QString& fileId);
    static std::pair<NetJob::Ptr, QString*> getChangelogTask(const QString& id, const QString& fileId);
    static Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> matchFingerprints(const QList<uint>& fingerprints,
                                                                                         bool onlyAvailable = false);
    static std::pair<NetJob::Ptr, QHash<QString, ModPlatform::IndexedVersion>*> matchFingerprintsTask(const QList<uint>& fingerprints,
                                                                                                      bool onlyAvailable = false);

   private:
    static QUrl searchProjectsURL(const SearchArgs& args);
    static QUrl getVersionsURL(const VersionSearchArgs& args);
};
