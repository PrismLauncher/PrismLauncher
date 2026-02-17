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

#include <list>
#include <optional>

#include "../Version.h"

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"
#include "tasks/Task.h"

/* Simple class with a common interface for interacting with APIs */
class ResourceAPI {
   public:
    virtual ~ResourceAPI() = default;

    struct SortingMethod {
        // The index of the sorting method. Used to allow for arbitrary ordering in the list of methods.
        // Used by Flame in the API request.
        unsigned int index;
        // The real name of the sorting, as used in the respective API specification.
        // Used by Modrinth in the API request.
        QString name;
        // The human-readable name of the sorting, used for display in the UI.
        QString readable_name;
    };

    template <typename T>
    struct Callback {
        std::function<void(T&)> on_succeed;
        std::function<void(QString const& reason, int network_error_code)> on_fail;
        std::function<void()> on_abort;
    };

    struct SearchArgs {
        ModPlatform::ResourceType type{};
        int offset = 0;

        std::optional<QString> search;
        std::optional<SortingMethod> sorting;
        std::optional<ModPlatform::ModLoaderTypes> loaders;
        std::optional<std::vector<Version>> versions;
        std::optional<ModPlatform::Side> side;
        std::optional<QStringList> categoryIds;
        bool openSource;
    };

    struct VersionSearchArgs {
        ModPlatform::IndexedPack::Ptr pack;

        std::optional<std::vector<Version>> mcVersions;
        std::optional<ModPlatform::ModLoaderTypes> loaders;
        ModPlatform::ResourceType resourceType;
    };

    struct ProjectInfoArgs {
        ModPlatform::IndexedPack::Ptr pack;
    };

    struct DependencySearchArgs {
        ModPlatform::Dependency dependency;
        Version mcVersion;
        ModPlatform::ModLoaderTypes loader;
    };

   public:
    /** Gets a list of available sorting methods for this API. */
    virtual auto getSortingMethods() const -> QList<SortingMethod> = 0;

   public slots:
    virtual Task::Ptr searchProjects(SearchArgs&&, Callback<QList<ModPlatform::IndexedPack::Ptr>>&&) const;

    virtual Task::Ptr getProject(QString addonId, QByteArray* response) const;
    virtual Task::Ptr getProjects(QStringList addonIds, QByteArray* response) const = 0;

    virtual Task::Ptr getProjectInfo(ProjectInfoArgs&&, Callback<ModPlatform::IndexedPack::Ptr>&&) const;
    Task::Ptr getProjectVersions(VersionSearchArgs&& args, Callback<QVector<ModPlatform::IndexedVersion>>&& callbacks) const;
    virtual Task::Ptr getDependencyVersion(DependencySearchArgs&&, Callback<ModPlatform::IndexedVersion>&&) const;

   protected:
    inline QString debugName() const { return "External resource API"; }

    QString mapMCVersionToModrinth(Version v) const;

    QString getGameVersionsString(std::vector<Version> mcVersions) const;

   public:
    virtual auto getSearchURL(SearchArgs const& args) const -> std::optional<QString> = 0;
    virtual auto getInfoURL(QString const& id) const -> std::optional<QString> = 0;
    virtual auto getVersionsURL(VersionSearchArgs const& args) const -> std::optional<QString> = 0;
    virtual auto getDependencyURL(DependencySearchArgs const& args) const -> std::optional<QString> = 0;

    /** Functions to load data into a pack.
     *
     *  Those are needed for the same reason as documentToArray, and NEED to be re-implemented in the same way.
     */

    virtual void loadIndexedPack(ModPlatform::IndexedPack&, QJsonObject&) const = 0;
    virtual ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, ModPlatform::ResourceType) const = 0;

    /** Converts a JSON document to a common array format.
     *
     *  This is needed so that different providers, with different JSON structures, can be parsed
     *  uniformally. You NEED to re-implement this if you intend on using the default callbacks.
     */
    virtual QJsonArray documentToArray(QJsonDocument& obj) const = 0;

    /** Functions to load data into a pack.
     *
     *  Those are needed for the same reason as documentToArray, and NEED to be re-implemented in the same way.
     */

    virtual void loadExtraPackInfo(ModPlatform::IndexedPack&, QJsonObject&) const = 0;
};
