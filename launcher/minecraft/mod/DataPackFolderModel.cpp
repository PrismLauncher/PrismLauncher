// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "DataPackFolderModel.h"

#include <QIcon>
#include <QStyle>

#include "minecraft/mod/tasks/LocalDataPackParseTask.h"

DataPackFolderModel::DataPackFolderModel(const QString& dir, BaseInstance* instance, bool isIndexed, bool createDir, QObject* parent)
    : ResourceFolderModel(QDir(dir), instance, isIndexed, createDir, parent)
{
    m_columnNames = QStringList({ "Enable", "Image", "Name", "Pack Format", "Last Modified", "Size", "File Name" });
    m_columnNamesTranslated =
        QStringList({ tr("Enable"), tr("Image"), tr("Name"), tr("Pack Format"), tr("Last Modified"), tr("Size"), tr("File Name") });
    m_columnSortKeys = { SortType::Enabled, SortType::Name, SortType::Name,    SortType::PackFormat,
                         SortType::Date,    SortType::Size, SortType::Filename };
    m_columnResizeModes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,    QHeaderView::Interactive,
                            QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive };
    m_columnsHideable = { false, true, false, true, true, true, true };
}

QVariant DataPackFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index)) {
        return {};
    }

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::BackgroundRole:
            return rowBackground(row);
        case Qt::DisplayRole:
            if (column == PackFormatColumn) {
                const auto& resource = at(row);
                return resource.packFormatStr();
            }
            if (column == SizeColumn) {
                const auto& resource = at(row);
                return resource.sizeStr();
            }
            break;
        case Qt::DecorationRole: {
            if (column == ImageColumn) {
                return at(row).image({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            break;
        }
        case Qt::ToolTipRole: {
            if (column == PackFormatColumn) {
                //: The string being explained by this is in the format: ID (Lower version - Upper version)
                return tr("The data pack format ID, as well as the Minecraft versions it was designed for.");
            }
            break;
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            break;
        default:
            break;
    }

    // map the columns to the base equivilents
    QModelIndex mappedIndex;
    switch (column) {
        case ActiveColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::ActiveColumn);
            break;
        case NameColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::NameColumn);
            break;
        case DateColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::DateColumn);
            break;
        case ProviderColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::ProviderColumn);
            break;
        case FileNameColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::FileNameColumn);
            break;
        case SizeColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::SizeColumn);
            break;
        default:
            break;
    }

    if (mappedIndex.isValid()) {
        return ResourceFolderModel::data(mappedIndex, role);
    }

    return {};
}

QVariant DataPackFolderModel::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                case NameColumn:
                case PackFormatColumn:
                case DateColumn:
                case ImageColumn:
                case SizeColumn:
                case FileNameColumn:
                    return columnNames().at(section);
                default:
                    return {};
            }

        case Qt::ToolTipRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Is the data pack enabled? (Only valid for ZIPs)");
                case NameColumn:
                    return tr("The name of the data pack.");
                case PackFormatColumn:
                    //: The string being explained by this is in the format: ID (Lower version - Upper version)
                    return tr("The data pack format ID, as well as the Minecraft versions it was designed for.");
                case DateColumn:
                    return tr("The date and time this data pack was last changed (or added).");
                case SizeColumn:
                    return tr("The size of the data pack.");
                case FileNameColumn:
                    return tr("The file name of the data pack.");
                default:
                    return {};
            }
        case Qt::SizeHintRole:
            if (section == ImageColumn) {
                return QSize(64, 0);
            }
            return {};
        default:
            return {};
    }
}

int DataPackFolderModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : NumColumns;
}

Resource* DataPackFolderModel::createResource(const QFileInfo& file)
{
    return new DataPack(file);
}

Task* DataPackFolderModel::createParseTask(Resource& resource)
{
    return new LocalDataPackParseTask(m_nextResolutionTicket, static_cast<DataPack*>(&resource));
}
