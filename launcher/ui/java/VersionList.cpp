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

#include "VersionList.h"

#include <memory>

#include "BaseVersionList.h"
#include "SysInfo.h"
#include "java/JavaMetadata.h"
#include "meta/VersionList.h"

namespace Java {

VersionList::VersionList(Meta::Version::Ptr version, QObject* parent) : BaseVersionList(parent), m_version(version)
{
    if (version->isLoaded())
        sortVersions();
}

Task::Ptr VersionList::getLoadTask()
{
    auto task = m_version->loadTask(Net::Mode::Online);
    connect(task.get(), &Task::finished, this, &VersionList::sortVersions);
    return task;
}

const BaseVersion::Ptr VersionList::at(int i) const
{
    return m_vlist.at(i);
}

bool VersionList::isLoaded()
{
    return m_version->isLoaded();
}

int VersionList::count() const
{
    return m_vlist.count();
}

QVariant VersionList::data(const QModelIndex& index, int role) const
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
            return false;  // do not recommend any version
        case JavaNameRole:
            return version->name();
        case JavaMajorRole: {
            auto major = version->version.toString();
            if (major.startsWith("java")) {
                major = "Java " + major.mid(4);
            }
            return major;
        }
        case TypeRole:
            return version->packageType;
        case Meta::VersionList::TimeRole:
            return version->releaseTime;
        default:
            return QVariant();
    }
}

BaseVersionList::RoleList VersionList::providesRoles() const
{
    return { VersionPointerRole, VersionIdRole, VersionRole, RecommendedRole, JavaNameRole, TypeRole, Meta::VersionList::TimeRole };
}

bool sortJavas(BaseVersion::Ptr left, BaseVersion::Ptr right)
{
    auto rleft = std::dynamic_pointer_cast<Java::Metadata>(right);
    auto rright = std::dynamic_pointer_cast<Java::Metadata>(left);
    return (*rleft) < (*rright);
}

void VersionList::sortVersions()
{
    if (!m_version || !m_version->data())
        return;
    QString versionStr = SysInfo::getSupportedJavaArchitecture();
    beginResetModel();
    auto runtimes = m_version->data()->runtimes;
    m_vlist = {};
    if (!versionStr.isEmpty() && !runtimes.isEmpty()) {
        std::copy_if(runtimes.begin(), runtimes.end(), std::back_inserter(m_vlist),
                     [versionStr](Java::MetadataPtr val) { return val->runtimeOS == versionStr; });
        std::sort(m_vlist.begin(), m_vlist.end(), sortJavas);
    } else {
        qWarning() << "No Java versions found for your operating system." << SysInfo::currentSystem() << " " << SysInfo::useQTForArch();
    }
    endResetModel();
}

}  // namespace Java
