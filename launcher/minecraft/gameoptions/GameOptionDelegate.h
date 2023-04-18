// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Tayou <tayou@gmx.net>
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
#include <QString>
#include <QWidget>

#include "GameOptions.h"

class GameOptionDelegate : public QStyledItemDelegate {
   public:
    GameOptionDelegate(QObject* parent, std::vector<GameOptionItem>* contents) : contents(contents), QStyledItemDelegate(parent)
    {
        keybindingOptions = GameOptionsSchema::getKeyBindOptions();
    };
    QWidget* createEditor(QWidget* parent,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor,
                                                   const QStyleOptionViewItem& option,
                                                   const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    enum GameOptionWidgetType { slider, text, keybind, number, comboBox };

   private:
    std::vector<GameOptionItem>* contents;
    QList<std::shared_ptr<KeyBindData>>* keybindingOptions;
};