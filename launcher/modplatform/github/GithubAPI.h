// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "modplatform/ResourceAPI.h"

#include <QJsonObject>
#include <QUrl>

#include "net/HeaderProxy.h"

namespace Github {

/** Base URL of the GitHub REST API. */
inline QUrl apiUrl()
{
    return { QStringLiteral("https://api.github.com") };
}

/** Per-page size used for repository searches. The resource model assumes pages of 25. */
inline int searchPageSize()
{
    return 25;
}

}  // namespace Github

class GithubAPI final : public ResourceAPI {
   public:
    static const GithubAPI& get()
    {
        static const GithubAPI s_instance;
        return s_instance;
    }

    auto getSortingMethods() const -> QList<ResourceAPI::SortingMethod> override;

    std::pair<Task::Ptr, QByteArray*> getProjects(QStringList addonIds) const override;

    std::pair<Task::Ptr, QByteArray*> getModCategories() const override;
    QList<ModPlatform::Category> loadModCategories(const QByteArray& response) const override;

   public slots:
    Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const override;
    Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const override;

    /** Headers required for every GitHub REST API request (User-Agent is mandatory). */
    static QList<Net::HeaderPair> headers();

    Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType type) const override;

   private:
    auto getVersionsURL(const VersionSearchArgs& args) const -> std::optional<QString> override;
    auto getDependencyURL(const DependencySearchArgs& args) const -> std::optional<QString> override;

    QString debugName() const override { return "GitHub"; }

    // GitHub has no batch project endpoint we can use through this interface, and no categories.
    // The generic download flow never calls getProjects/getModCategories for this provider, so we
    // answer both with an empty payload.
    mutable QByteArray m_emptyResponse{ "[]" };
};
