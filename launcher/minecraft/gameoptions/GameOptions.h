// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Tayou <tayou@gmx.net>
 *  Copyright (C) 2023 TheLastRar <TheLastRar@users.noreply.github.com>
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
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <map>

#include "GameOptionsSchema.h"

struct GameOptionChildItem {
    QString value;
    int parentRow;
};

struct GameOptionItem {
    QString key;
    bool boolValue;
    int intValue;
    float floatValue;
    QString value;
    OptionType type;
    std::shared_ptr<GameOption> knownOption;
    QList<GameOptionChildItem> children;
};

class GameOptions : public QAbstractItemModel {
    Q_OBJECT
   public:
    enum class Column { Key, Description, Value };
    explicit GameOptions(const QString& path);
    virtual ~GameOptions() = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    bool isLoaded() const;
    bool reload();
    bool save();

    std::vector<GameOptionItem>* getContents(){ return &contents; };

   private:
    std::vector<GameOptionItem> contents;
    bool loaded = false;
    QString path;
    int version = 0;

    QMap<QString, std::shared_ptr<GameOption>>* knownOptions{};
    QList<std::shared_ptr<KeyBindData>>* keybindingOptions{};
};
