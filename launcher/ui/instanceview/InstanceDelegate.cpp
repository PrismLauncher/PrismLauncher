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

InstanceDelegate::InstanceDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void InstanceDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() == InstanceList::NameColumn) {
        // make decoration fill cell, subtract default margins
        QSize decorationSize = QSize(option->rect.height(), option->rect.height());
        decorationSize -= QSize(2, 2);  // subtract 1px margin
        option->decorationSize = decorationSize;
    }
}