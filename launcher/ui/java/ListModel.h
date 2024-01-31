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

#pragma once

#include "java/JavaRuntime.h"
#include "meta/VersionList.h"

namespace Java {

class JavaBaseVersionList : public Meta::VersionList {
    Q_OBJECT
   public:
    explicit JavaBaseVersionList(const QString& uid, QObject* parent = nullptr) : VersionList(uid, parent) {}
    BaseVersionList::RoleList providesRoles() const { return { VersionRole, RecommendedRole, VersionPointerRole }; }
};

class InstallList : public BaseVersionList {
    Q_OBJECT

   public:
    explicit InstallList(Meta::Version::Ptr m_version, QObject* parent = 0);

    Task::Ptr getLoadTask() override;
    bool isLoaded() override;
    const BaseVersion::Ptr at(int i) const override;
    int count() const override;
    void sortVersions() override;

    QVariant data(const QModelIndex& index, int role) const override;
    RoleList providesRoles() const override;

   protected slots:
    void updateListData(QList<BaseVersion::Ptr>) override {}

   protected:
    Meta::Version::Ptr m_version;
    QList<JavaRuntime::MetaPtr> m_vlist;
};

}  // namespace Java
