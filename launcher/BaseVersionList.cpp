// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "BaseVersionList.h"
#include "BaseVersion.h"

BaseVersionList::BaseVersionList(QObject* parent) : QAbstractListModel(parent) {}

BaseVersion::Ptr BaseVersionList::findVersion(const QString& descriptor)
{
    for (int i = 0; i < count(); i++) {
        if (at(i)->descriptor() == descriptor)
            return at(i);
    }
    return nullptr;
}

BaseVersion::Ptr BaseVersionList::getRecommended() const
{
    if (count() <= 0)
        return nullptr;
    else
        return at(0);
}

QVariant BaseVersionList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > count())
        return QVariant();

    BaseVersion::Ptr version = at(index.row());

    switch (role) {
        case VersionPointerRole:
            return QVariant::fromValue(version);

        case VersionRole:
            return version->name();

        case VersionIdRole:
            return version->descriptor();

        case TypeRole:
            return version->typeString();

        case JavaMajorRole: {
            auto major = version->name();
            if (major.startsWith("java")) {
                major = "Java " + major.mid(4);
            }
            return major;
        }

        default:
            return QVariant();
    }
}

BaseVersionList::RoleList BaseVersionList::providesRoles() const
{
    return { VersionPointerRole, VersionRole, VersionIdRole, TypeRole };
}

int BaseVersionList::rowCount(const QModelIndex& parent) const
{
    // Return count
    return parent.isValid() ? 0 : count();
}

int BaseVersionList::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QHash<int, QByteArray> BaseVersionList::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(VersionRole, "version");
    roles.insert(VersionIdRole, "versionId");
    roles.insert(ParentVersionRole, "parentGameVersion");
    roles.insert(RecommendedRole, "recommended");
    roles.insert(LatestRole, "latest");
    roles.insert(TypeRole, "type");
    roles.insert(BranchRole, "branch");
    roles.insert(PathRole, "path");
    roles.insert(JavaNameRole, "javaName");
    roles.insert(CPUArchitectureRole, "architecture");
    roles.insert(JavaMajorRole, "javaMajor");
    return roles;
}
