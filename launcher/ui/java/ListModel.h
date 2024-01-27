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

#include <qtmetamacros.h>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include "java/JavaRuntime.h"
#include "meta/VersionList.h"

namespace Java {

class JavaBaseVersionList : public Meta::VersionList {
    Q_OBJECT
   public:
    explicit JavaBaseVersionList(const QString& uid, QObject* parent = nullptr) : VersionList(uid, parent) {}
    BaseVersionList::RoleList providesRoles() const { return { VersionRole, RecommendedRole, VersionPointerRole }; }
};

struct JavaRuntime2 : public BaseVersion {
    JavaRuntime2() {}
    JavaRuntime2(JavaRuntime::MetaPtr m) : meta(m) {}
    virtual QString descriptor() override { return meta->version.toString(); }

    virtual QString name() override { return meta->name; }

    virtual QString typeString() const override { return meta->vendor; }

    virtual bool operator<(BaseVersion& a) override;
    virtual bool operator>(BaseVersion& a) override;
    bool operator<(const JavaRuntime2& rhs);
    bool operator==(const JavaRuntime2& rhs);
    bool operator>(const JavaRuntime2& rhs);

    JavaRuntime::MetaPtr meta;
};

using JavaRuntimePtr = std::shared_ptr<JavaRuntime2>;

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
    QList<JavaRuntimePtr> m_vlist;
};

}  // namespace Java
// class FilterModel : public QSortFilterProxyModel {
//     Q_OBJECT
//    public:
//     FilterModel(QObject* parent = Q_NULLPTR);
//     enum Sorting { ByName, ByGameVersion };
//     const QMap<QString, Sorting> getAvailableSortings();
//     QString translateCurrentSorting();
//     void setSorting(Sorting sorting);
//     Sorting getCurrentSorting();
//     void setSearchTerm(QString term);

//    protected:
//     bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
//     bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

//    private:
//     QMap<QString, Sorting> sortings;
//     Sorting currentSorting;
//     QString searchTerm;
// };

// class ListModel : public QAbstractListModel {
//     Q_OBJECT
//    private:
//     ModpackList modpacks;
//     QStringList m_failedLogos;
//     QStringList m_loadingLogos;
//     FTBLogoMap m_logoMap;
//     QMap<QString, LogoCallback> waitingCallbacks;

//     void requestLogo(QString file);
//     QString translatePackType(PackType type) const;

//    private slots:
//     void logoFailed(QString logo);
//     void logoLoaded(QString logo, QIcon out);

//    public:
//     ListModel(QObject* parent);
//     ~ListModel();
//     int rowCount(const QModelIndex& parent) const override;
//     int columnCount(const QModelIndex& parent) const override;
//     QVariant data(const QModelIndex& index, int role) const override;
//     Qt::ItemFlags flags(const QModelIndex& index) const override;

//     void fill(ModpackList modpacks);
//     void addPack(Modpack modpack);
//     void clear();
//     void remove(int row);

//     Modpack at(int row);
//     void getLogo(const QString& logo, LogoCallback callback);
// };
