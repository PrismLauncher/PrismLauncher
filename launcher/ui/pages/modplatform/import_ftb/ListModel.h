// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include <QAbstractListModel>
#include <QIcon>
#include <QSortFilterProxyModel>
#include <QVariant>
#include "modplatform/import_ftb/PackHelpers.h"

namespace FTBImportAPP {

class FilterModel : public QSortFilterProxyModel {
    Q_OBJECT
   public:
    FilterModel(QObject* parent = Q_NULLPTR);
    enum Sorting { ByName, ByGameVersion };
    const QMap<QString, Sorting> getAvailableSortings();
    QString translateCurrentSorting();
    void setSorting(Sorting sorting);
    Sorting getCurrentSorting();
    void setSearchTerm(QString term);

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

   private:
    QMap<QString, Sorting> sortings;
    Sorting currentSorting;
    QString searchTerm;
};

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(QObject* parent) : QAbstractListModel(parent) {}
    virtual ~ListModel() = default;

    int rowCount(const QModelIndex& parent) const { return modpacks.size(); }
    int columnCount(const QModelIndex& parent) const { return 1; }
    QVariant data(const QModelIndex& index, int role) const;

    void update();

    static const QString FTB_APP_PATH;

   private:
    ModpackList modpacks;
};
}  // namespace FTBImportAPP