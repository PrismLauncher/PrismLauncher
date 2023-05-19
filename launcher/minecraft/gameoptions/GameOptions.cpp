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

#include <QDebug>
#include <QSaveFile>
#include "FileSystem.h"

#include "FileSystem.h"
#include "GameOptions.h"

namespace {
bool load(const QString& path,
          std::vector<GameOptionItem>& contents,
          int& version,
          QMap<QString, std::shared_ptr<GameOption>>* &knownOptions,
          QList<std::shared_ptr<KeyBindData>>* &keybindingOptions)
{
    contents.clear();
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to read options file.";
        return false;
    }

    knownOptions = GameOptionsSchema::getKnownOptions();
    keybindingOptions = GameOptionsSchema::getKeyBindOptions();

    version = 0;
    while (!file.atEnd()) {
        // This should be handled by toml++ or some other toml parser rather than this manual parsing
        QString line = QString::fromUtf8(file.readLine());
        if (line.endsWith('\n')) {
            line.chop(1);
        }
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        GameOptionItem item = GameOptionItem();

        auto parts = line.split(':');

        item.key = parts[0];
        item.value = parts[1];
        item.type = OptionType::String;
        qDebug() << "Reading Game Options Key:" << item.key;

        if (item.key == "version") {
            version = item.value.toInt();
            continue;
        };

        bool isInt = false;
        bool isFloat = false;
        item.intValue = item.value.toInt(&isInt);
        item.floatValue = item.value.toFloat(&isFloat);
        item.boolValue = item.value == "true";
        item.type = isInt ? OptionType::Int : isFloat ? OptionType::Float : OptionType::Bool;
        if (item.value.startsWith("[") && item.value.endsWith("]")) {
            qDebug() << "The value" << item.value << "is an array";
            for (const QString& part : item.value.mid(1, item.value.size() - 2).split(",")) {
                GameOptionChildItem child{ part, static_cast<int>(contents.size()) };
                qDebug() << "Array has entry" << part;
                item.children.append(child);
            }
            item.type = OptionType::Array;
        } else if (item.key.startsWith("key_")) {
            item.type = OptionType::KeyBind;
        } else {
            // This removes the leading and ending " from the string to display it more cleanly.
            item.value = item.value.mid(1, item.value.length()-2);
            item.type = OptionType::String;
        }

        // adds reference to known option from gameOptionsSchema if available to get display name and other metadata
        item.knownOption = knownOptions->value(item.key, nullptr);
        contents.emplace_back(item);
    }
    qDebug() << "Loaded" << path << "with version:" << version;
    return true;
}
bool save(const QString& path, std::vector<GameOptionItem>& mapping, int version)
{
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (version != 0) {
        QString versionLine = QString("version:%1\n").arg(version);
        out.write(versionLine.toUtf8());
    }
    auto iter = mapping.begin();
    while (iter != mapping.end()) {
        out.write(iter->key.toUtf8());
        out.write(":");
        out.write(iter->value.toUtf8());
        out.write("\n");
        iter++;
    }
    return out.commit();
}
}  // namespace

GameOptions::GameOptions(const QString& path) : path(path)
{
    reload();
}

QVariant GameOptions::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
    switch ((Column)section) {
        default:
            return QVariant();
        case Column::Key:
            return tr("Key");
        case Column::Description:
            return tr("Description");
        case Column::Value:
            return tr("Value");
    }
}
bool GameOptions::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto row = index.row();
    auto column = (Column)index.column();
    if (column == Column::Value) {
        switch (contents[row].type) {
            case OptionType::Array:
            case OptionType::String: {
                contents[row].value = value.toString();
                return true;
            }
            case OptionType::Int: {
                contents[row].intValue = value.toInt();
                return true;
            }
            case OptionType::Bool: {
                contents[row].boolValue = value.toBool();
                return true;
            }
            case OptionType::Float: {
                contents[row].floatValue = value.toFloat();
                return true;
            }
            case OptionType::KeyBind:
                break;
        }
    }

    return true;
}

Qt::ItemFlags GameOptions::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index);
}

QVariant GameOptions::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    int row = index.row();
    Column column = (Column)index.column();

    if (row < 0 || row >= int(contents.size()))
        return {};

    if (index.parent().isValid()) {
        switch (role) {
            case Qt::DisplayRole: {
                if (column == Column::Value) {
                    GameOptionChildItem* item = static_cast<GameOptionChildItem*>(index.internalPointer());
                    return item->value;
                } else {
                    return {};
                }
            }
            default: {
                return {};
            }
        }
    }

    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case Column::Key: {
                    return contents[row].key;
                }
                case Column::Description: {
                    if (contents[row].knownOption != nullptr) {
                        return contents[row].knownOption->description;
                    } else {
                        return {};
                    }
                }
                default: {
                    return {};
                }
            }
        }
        default: {
            return {};
        }
    }
    return {};
}

QModelIndex GameOptions::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    if (parent.isValid()) {
        if (parent.parent().isValid())
            return {};

        GameOptionItem* item = static_cast<GameOptionItem*>(parent.internalPointer());
        return createIndex(row, column, &item->children[row]);
    } else {
        return createIndex(row, column, reinterpret_cast<quintptr>(&contents[row]));
    }
}

QModelIndex GameOptions::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    const void* childItem = index.internalPointer();

    // Determine where childItem points to
    if (childItem >= &contents[0] && childItem <= &contents.back()) {
        // Parent is root/contents
        return {};
    } else {
        GameOptionChildItem* child = static_cast<GameOptionChildItem*>(index.internalPointer());
        return createIndex(child->parentRow, 0, reinterpret_cast<quintptr>(&contents[child->parentRow]));
    }
}

int GameOptions::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(contents.size());
    } else {
        if (parent.column() > 0)
            return 0;

        // Our tree model is only one layer deep
        // If we have parent, we can't go deeper
        if (parent.parent().isValid())
            return 0;

        auto* item = static_cast<GameOptionItem*>(parent.internalPointer());
        return item->children.count();
    }
}

int GameOptions::columnCount(const QModelIndex& parent) const
{
    // Our tree model is only one layer deep
    // If we have parent, we can't go deeper
    if (parent.parent().isValid())
        return 0;

    return 3;
}

bool GameOptions::isLoaded() const
{
    return loaded;
}

bool GameOptions::reload()
{
    beginResetModel();
    loaded = load(path, contents, version, knownOptions, keybindingOptions);
    endResetModel();
    return loaded;
}

bool GameOptions::save()
{
    return ::save(path, contents, version);
}
