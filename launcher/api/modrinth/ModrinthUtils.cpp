// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ModrinthUtils.h"

namespace ModrinthUtils {

QStringList getModLoaderStrings(Platform::ModLoaders types)
{
    QStringList l;
    for (auto loader : { Platform::ModLoader::NeoForge, Platform::ModLoader::Forge, Platform::ModLoader::Fabric, Platform::ModLoader::Quilt,
                         Platform::ModLoader::LiteLoader }) {
        if (types & loader) {
            l << Platform::ModloaderUtils::toString(loader);
        }
    }
    return l;
}

QString getModLoaderFilters(Platform::ModLoaders types)
{
    QStringList l;
    for (auto loader : getModLoaderStrings(types)) {
        l << QString("\"categories:%1\"").arg(loader);
    }
    return l.join(',');
}

QString getCategoriesFilters(QStringList categories)
{
    QStringList l;
    for (auto cat : categories) {
        l << QString("\"categories:%1\"").arg(cat);
    }
    return l.join(',');
}

QString getSideFilters(Platform::Side side)
{
    switch (side) {
        case Platform::Side::ClientSide:
            return QString("\"client_side:required\",\"client_side:optional\"],[\"server_side:optional\",\"server_side:unsupported\"");
        case Platform::Side::ServerSide:
            return QString("\"server_side:required\",\"server_side:optional\"],[\"client_side:optional\",\"client_side:unsupported\"");
        case Platform::Side::UniversalSide:
            return QString("\"client_side:required\"],[\"server_side:required\"");
        case Platform::Side::NoSide:
        // fallthrough
        default:
            return {};
    }
}

QString mapMCVersionToModrinth(Version v)
{
    static const QString preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(preString)) {
        verStr.replace(preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
}

QString getGameVersionsArray(std::list<Version> mcVersions)
{
    QString s;
    for (auto& ver : mcVersions) {
        s += QString("\"versions:%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s.isEmpty() ? QString() : s;
}

QString getGameVersionsString(std::list<Version> mcVersions)
{
    QString s;
    for (auto& ver : mcVersions) {
        s += QString("\"%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s;
}

QString resourceTypeParameter(Platform::ResourceType type)
{
    switch (type) {
        case Platform::ResourceType::Mod:
            return "mod";
        case Platform::ResourceType::ResourcePack:
            return "resourcepack";
        case Platform::ResourceType::ShaderPack:
            return "shader";
        case Platform::ResourceType::Modpack:
            return "modpack";
        default:
            qWarning() << "Invalid resource type for Modrinth API!";
            break;
    }

    return "";
}

QString createFacets(API::SearchArgs const& args)
{
    QStringList facets_list;

    if (args.loaders.has_value() && args.loaders.value() != 0)
        facets_list.append(QString("[%1]").arg(getModLoaderFilters(args.loaders.value())));
    if (args.versions.has_value() && !args.versions.value().empty())
        facets_list.append(QString("[%1]").arg(getGameVersionsArray(args.versions.value())));
    if (args.side.has_value()) {
        auto side = getSideFilters(args.side.value());
        if (!side.isEmpty())
            facets_list.append(QString("[%1]").arg(side));
    }
    if (args.categoryIds.has_value() && !args.categoryIds->empty())
        facets_list.append(QString("[%1]").arg(getCategoriesFilters(args.categoryIds.value())));
    if (args.openSource)
        facets_list.append("[\"open_source:true\"]");

    facets_list.append(QString("[\"project_type:%1\"]").arg(resourceTypeParameter(args.type)));

    return QString("[%1]").arg(facets_list.join(','));
}

bool shouldDownloadOnSide(QString side)
{
    return side == "required" || side == "optional";
}
}  // namespace ModrinthUtils
