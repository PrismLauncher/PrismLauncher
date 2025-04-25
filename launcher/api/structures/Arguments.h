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

#include <QString>

#include "../../Version.h"
#include "api/structures/Category.h"
#include "api/structures/ModLoader.h"
#include "api/structures/Project.h"
#include "api/structures/ResourceType.h"
#include "api/structures/Side.h"
#include "api/structures/SortingMethod.h"
#include "modplatform/helpers/HashUtils.h"

namespace API {
template <typename T>
struct Callback {
    std::function<void(T&)> on_succeed;
    std::function<void(QString const& reason, int network_error_code)> on_fail;
    std::function<void()> on_abort;
};

struct SearchArgs {
    Platform::ResourceType type{};
    int offset = 0;

    std::optional<QString> search;
    std::optional<SortingMethod> sorting;
    std::optional<Platform::ModLoaders> loaders;
    std::optional<std::list<Version>> versions;
    std::optional<Platform::Side> side;
    std::optional<QStringList> categoryIds;
    bool openSource;
};

struct VersionSearchArgs {
    Platform::Project pack;

    std::optional<std::list<::Version>> mcVersions;
    std::optional<Platform::ModLoaders> loaders;

    Platform::ResourceType resourceType{};
};

struct VersionSearchResponse {
    QList<Platform::Version> versions;
    QVariant projectId;
    Platform::ResourceType resourceType{};
};

struct ProjectInfoArgs {
    Platform::ResourceType resourceType{};
    Platform::Project::Ptr pack;
};

struct DependencySearchArgs {
    Platform::Dependency dependency;
    Platform::ModLoaders loader;
    Version mcVersion;
};

struct MatchHashesArgs {
    QStringList hashes;
    Hashing::Algorithm alg;
};

using MatchHashesResponse = QHash<QString, Platform::Version>;

struct CategoriesResponse {
    QList<Platform::Category> categories;
    Platform::ResourceType resourceType{};
};

}  // namespace API