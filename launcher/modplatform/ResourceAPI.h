// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QDebug>
#include <QList>
#include <QString>

#include <optional>
#include <utility>

#include "../Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"
#include "net/NetJob.h"
#include "net/RPCSink.h"

/* Simple class with a common interface for interacting with APIs */
class ResourceAPI {
   public:
    struct SortingMethod {
        // The index of the sorting method. Used to allow for arbitrary ordering in the list of methods.
        // Used by Flame in the API request.
        unsigned int index;
        // The real name of the sorting, as used in the respective API specification.
        // Used by Modrinth in the API request.
        QString name;
        // The human-readable name of the sorting, used for display in the UI.
        QString readableName;
    };

    struct SearchArgs {
        ModPlatform::ResourceType type{};
        int offset = 0;

        std::optional<QString> search;
        std::optional<SortingMethod> sorting;
        std::optional<ModPlatform::ModLoaderTypes> loaders;
        std::optional<std::vector<Version>> versions;
        std::optional<ModPlatform::SideType> side;
        std::optional<QStringList> categoryIds;
        bool openSource{};
        std::vector<ModPlatform::DisclosureType> excludeDisclosureTypes;
    };

    struct VersionSearchArgs {
        ModPlatform::IndexedPack::Ptr pack;

        std::optional<std::vector<Version>> mcVersions;
        std::optional<ModPlatform::ModLoaderTypes> loaders;
        ModPlatform::ResourceType resourceType;
        bool includeChangelog{};
    };

   public:
    /** Gets a list of available sorting methods for this API. */
    virtual auto getSortingMethods() const -> QList<SortingMethod> = 0;

   public slots:

    virtual Net::RPC::Spec<ModPlatform::IndexedPack> getProject(const QString& id) const = 0;
    virtual Net::RPC::Spec<QList<ModPlatform::IndexedPack>> getProjects(const QStringList& addonIds) const = 0;
    virtual std::optional<Net::RPC::Spec<bool>> getProjectExtra(ModPlatform::IndexedPack& /*pack*/) const { return {}; }
    virtual Net::RPC::Spec<QList<ModPlatform::IndexedPack>> searchProjects(const SearchArgs& args) const = 0;
    virtual Net::RPC::Spec<QList<ModPlatform::Category>> getCategories(ModPlatform::ResourceType type) const = 0;
    virtual Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const VersionSearchArgs& args) const = 0;
    virtual Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> getVersions(const QStringList& versionIds) const = 0;
    virtual Net::RPC::Spec<ModPlatform::IndexedVersion> getVersion(const QString& id, const QString& versionId) const = 0;

    // helpers to omit the netJob stuff
    std::pair<NetJob::Ptr, ModPlatform::IndexedPack*> getProjectTask(const QString& addonId,
                                                                     bool loadExtra = false,
                                                                     bool askRetry = true) const;
    std::pair<NetJob::Ptr, QList<ModPlatform::IndexedPack>*> searchProjectsTask(const SearchArgs& args) const;
    std::pair<NetJob::Ptr, QList<ModPlatform::IndexedPack>*> getProjectsTask(const QStringList& addonIds) const;
    std::pair<NetJob::Ptr, QList<ModPlatform::Category>*> getCategoriesTask(ModPlatform::ResourceType type) const;
    std::pair<NetJob::Ptr, QList<ModPlatform::IndexedVersion>*> getVersionsTask(const VersionSearchArgs& args) const;
    std::pair<NetJob::Ptr, QList<ModPlatform::IndexedVersion>*> getVersionsTask(const QStringList& versionIds) const;
    std::pair<NetJob::Ptr, ModPlatform::IndexedVersion*> getVersionTask(const QString& id, const QString& versionId) const;

   protected:
    ~ResourceAPI() = default;

    virtual QString debugName() const { return "External resource API"; }
};
