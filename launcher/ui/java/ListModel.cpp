// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ListModel.h"

#include <memory>

#include "BaseVersionList.h"
#include "SysInfo.h"
#include "java/JavaRuntime.h"

namespace Java {

InstallList::InstallList(Meta::Version::Ptr version, QObject* parent) : BaseVersionList(parent), m_version(version)
{
    if (version->isLoaded())
        sortVersions();
}

Task::Ptr InstallList::getLoadTask()
{
    m_version->load(Net::Mode::Online);
    auto task = m_version->getCurrentTask();
    connect(task.get(), &Task::finished, this, &InstallList::sortVersions);
    return task;
}

const BaseVersion::Ptr InstallList::at(int i) const
{
    return m_vlist.at(i);
}

bool InstallList::isLoaded()
{
    return m_version->isLoaded();
}

int InstallList::count() const
{
    return m_vlist.count();
}

QVariant InstallList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > count())
        return QVariant();

    auto version = (m_vlist[index.row()]);
    switch (role) {
        case SortRole:
            return -index.row();
        case VersionPointerRole:
            return QVariant::fromValue(std::dynamic_pointer_cast<BaseVersion>(m_vlist[index.row()]));
        case VersionIdRole:
            return version->descriptor();
        case VersionRole:
            return version->version.toString();
        case RecommendedRole:
            return version->recommended;
        case AliasRole:
            return version->name();
        case ArchitectureRole:
            return version->vendor;
        default:
            return QVariant();
    }
}

BaseVersionList::RoleList InstallList::providesRoles() const
{
    return { VersionPointerRole, VersionIdRole, VersionRole, RecommendedRole, AliasRole, ArchitectureRole };
}

bool sortJavas(BaseVersion::Ptr left, BaseVersion::Ptr right)
{
    auto rleft = std::dynamic_pointer_cast<JavaRuntime::Meta>(right);
    auto rright = std::dynamic_pointer_cast<JavaRuntime::Meta>(left);
    return (*rleft) > (*rright);
}

void InstallList::sortVersions()
{
    QString versionStr = SysInfo::getSupportedJavaArchitecture();
    beginResetModel();
    auto runtimes = m_version->data()->runtimes;
    if (!versionStr.isEmpty() && runtimes.contains(versionStr)) {
        m_vlist = runtimes.value(versionStr);
        std::sort(m_vlist.begin(), m_vlist.end(), sortJavas);
    } else {
        m_vlist = {};
    }
    endResetModel();
}

}  // namespace Java
