// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 TheKodeToad <TheKodeToad@proton.me>
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

#include "LogFormatProxyModel.h"

#include "Application.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

QVariant LogFormatProxyModel::data(const QModelIndex& index, int role) const
{
    const LogColors& colors = APPLICATION->themeManager()->getLogColors();

    switch (role) {
        case Qt::FontRole:
            return m_font;
        case Qt::ForegroundRole: {
            auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
            QColor result = colors.foreground.value(level);

            if (result.isValid())
                return result;

            break;
        }
        case Qt::BackgroundRole: {
            auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
            QColor result = colors.background.value(level);

            if (result.isValid())
                return result;

            break;
        }
    }

    return QIdentityProxyModel::data(index, role);
}

QModelIndex LogFormatProxyModel::find(const QModelIndex& start, const QString& value, bool reverse) const
{
    QModelIndex parentIndex = parent(start);
    auto compare = [this, start, parentIndex, value](int r) -> QModelIndex {
        QModelIndex idx = index(r, start.column(), parentIndex);
        if (!idx.isValid() || idx == start) {
            return QModelIndex();
        }
        QVariant v = data(idx, Qt::DisplayRole);
        QString t = v.toString();
        if (t.contains(value, Qt::CaseInsensitive))
            return idx;
        return QModelIndex();
    };
    if (reverse) {
        int from = start.row();
        int to = 0;

        for (int i = 0; i < 2; ++i) {
            for (int r = from; (r >= to); --r) {
                auto idx = compare(r);
                if (idx.isValid())
                    return idx;
            }
            // prepare for the next iteration
            from = rowCount() - 1;
            to = start.row();
        }
    } else {
        int from = start.row();
        int to = rowCount(parentIndex);

        for (int i = 0; i < 2; ++i) {
            for (int r = from; (r < to); ++r) {
                auto idx = compare(r);
                if (idx.isValid())
                    return idx;
            }
            // prepare for the next iteration
            from = 0;
            to = start.row();
        }
    }
    return QModelIndex();
}