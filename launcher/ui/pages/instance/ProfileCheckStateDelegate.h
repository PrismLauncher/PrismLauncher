// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vivek Kushwaha <notvivekkushwaha@gmail.com>
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

#include <QStyledItemDelegate>
#include <QSet>
#include <QString>
#include <QMap>

class ModFolderModel;
class QSortFilterProxyModel;

class ProfileCheckStateDelegate : public QStyledItemDelegate {
    Q_OBJECT
   public:
    explicit ProfileCheckStateDelegate(
        const QMap<QString, QSet<QString>>* profileStates,
        const QString* currentProfile,
        QSortFilterProxyModel* filterModel,
        ModFolderModel* model,
        QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    void initStyleOption(QStyleOptionViewItem* option,
                         const QModelIndex& index) const override;

    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

   signals:
    void membershipToggled(const QString& modId, bool enabled);

   private:
    QString resolveModId(const QModelIndex& proxyIndex) const;

    const QMap<QString, QSet<QString>>* m_profileStates;
    const QString* m_currentProfile;
    QSortFilterProxyModel* m_filterModel;
    ModFolderModel* m_model;
};
