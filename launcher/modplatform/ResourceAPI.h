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

#include "api/Api.h"
#include "api/flame/FlameAPI.h"
#include "api/modrinth/ModrinthAPI.h"
#include "api/structures/Arguments.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "api/structures/ResourceType.h"
#include "api/structures/SortingMethod.h"
#include "tasks/Task.h"

/* Simple class with a common interface for interacting with APIs */
class ResourceAPI {
   public:
    ResourceAPI()
    {
        API::FlameAPI::registerClass();
        API::ModrinthAPI::registerClass();
    }
    virtual ~ResourceAPI() = default;

   public:
    /** Gets a list of available sorting methods for this API. */
    [[nodiscard]] QList<API::SortingMethod> getSortingMethods() const { return API::ProviderAPI::get(provider())->getSortingMethods(); }

   public slots:
    [[nodiscard]] Task::Ptr searchProjects(API::SearchArgs&&, API::Callback<QList<Platform::Project::Ptr>>&&) const;

    [[nodiscard]] Task::Ptr getProjectInfo(API::ProjectInfoArgs&&, API::Callback<Platform::Project>&&) const;
    [[nodiscard]] Task::Ptr getProjectVersions(API::VersionSearchArgs&& args, API::Callback<QVector<Platform::Version>>&& callbacks) const;
    [[nodiscard]] Task::Ptr getDependencyVersion(API::DependencySearchArgs&&, API::Callback<Platform::Version>&&) const;

   protected:
    [[nodiscard]] inline QString debugName() const { return "External resource API"; }

   public:
    virtual Platform::Provider provider() const = 0;
};
