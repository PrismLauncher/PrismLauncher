// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "InstanceDelegate.h"
#include "InstanceList.h"

InstanceDelegate::InstanceDelegate(QObject* parent, int iconSize, bool isGrid)
    : QStyledItemDelegate(parent), m_iconSize(iconSize), m_isGrid(isGrid)
{}

void InstanceDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() == InstanceList::NameColumn) {
        option->decorationSize = QSize(m_iconSize, m_iconSize);
        if (m_isGrid) {
            // FIXME: kinda hacky way to add vertical padding. This assumes that the icon is square in the first place
            option->decorationSize.rheight() += 8;
        }
    }
}
