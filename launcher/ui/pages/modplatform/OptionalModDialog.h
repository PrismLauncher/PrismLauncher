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
#include <QDialog>

namespace Ui {
class OptionalModDialog;
}

class OptionalModListModel : public QAbstractListModel {
    Q_OBJECT
   public:
    enum Columns {
        EnabledColumn = 0,
        NameColumn,
    };

    OptionalModListModel(QWidget* parent, QStringList mods);

    QStringList getResult();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

   public slots:
    void selectAll() { toggleAll(true); }
    void clearAll() { toggleAll(false); };
    void toggleAll(bool enabled);

   private:
    void toggleMod(QString mod, int index);
    void setMod(QString mod, int index, bool enable, bool shouldEmit = true);

   private:
    QStringList m_mods;
    QHash<QString, bool> m_selected;
};

class OptionalModDialog : public QDialog {
    Q_OBJECT

   public:
    OptionalModDialog(QWidget* parent, QStringList mods);
    ~OptionalModDialog() override;

    QStringList getResult() { return listModel->getResult(); }

   private:
    Ui::OptionalModDialog* ui;

    OptionalModListModel* listModel;
};
