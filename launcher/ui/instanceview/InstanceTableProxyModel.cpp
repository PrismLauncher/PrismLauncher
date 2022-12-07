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

#include "InstanceTableProxyModel.h"

#include <icons/IconList.h>
#include "Application.h"
#include "InstanceList.h"

#include <QFont>
#include <QRegularExpression>
#include <QVariant>

InstanceTableProxyModel::InstanceTableProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    m_naturalSort.setNumericMode(true);
    m_naturalSort.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    // FIXME: use loaded translation as source of locale instead, hook this up to translation changes
    m_naturalSort.setLocale(QLocale::system());
}

QVariant InstanceTableProxyModel::data(const QModelIndex& index, int role) const
{
    QVariant data = QSortFilterProxyModel::data(index, role);
    QVariant displayData = data;
    if (role != Qt::DisplayRole)
        displayData = QSortFilterProxyModel::data(index, Qt::DisplayRole);

    switch (role) {
        case Qt::DecorationRole: {
            QString iconKey = data.toString();
            if (iconKey.isEmpty())
                break;

            switch (index.column()) {
                case InstanceList::StatusColumn:
                    return QIcon::fromTheme(iconKey);
                case InstanceList::NameColumn:
                    return APPLICATION->icons()->getIcon(iconKey);
            }
            break;
        }
        case Qt::DisplayRole: {
            switch (index.column()) {
                case InstanceList::CategoryColumn: {
                    if (data.toString().isEmpty())
                        return tr("None");
                    break;
                }
                case InstanceList::LastPlayedColumn: {
                    QDateTime foo = data.toDateTime();
                    if (foo.isNull() || !foo.isValid() || foo.toMSecsSinceEpoch() == 0)
                        return tr("Never");
                    break;
                }
            }
            break;
        }
        case Qt::FontRole: {
            QFont font = data.value<QFont>();
            switch (index.column()) {
                case InstanceList::CategoryColumn: {
                    if (displayData.toString().isEmpty())
                        font.setItalic(true);
                    break;
                }
                case InstanceList::LastPlayedColumn: {
                    QDateTime foo = data.toDateTime();
                    if (foo.isNull() || !foo.isValid() || foo.toMSecsSinceEpoch() == 0)
                        font.setItalic(true);
                    break;
                }
            }
            return font;
        }
    }
    return data;
}

void InstanceTableProxyModel::setFilterQuery(const QString query)
{
    QList<InstanceFilterQuery> foo = parseFilterQuery(query);
    setFilterQuery(foo);
}

void InstanceTableProxyModel::setFilterQuery(const QList<InstanceFilterQuery> query)
{
    m_filter = query;
    invalidateFilter();
}

bool InstanceTableProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_filter.isEmpty())
        return true;

    for (const InstanceFilterQuery& q : m_filter) {
        const InstanceList::Column c = q.first;
        const QString query = q.second;

        QModelIndex index = sourceModel()->index(sourceRow, c, sourceParent);
        QString content = sourceModel()->data(index).toString().toLower();

        if (!query.isNull() && !content.contains(query))
            return false;
    }
    return true;
}

QList<InstanceFilterQuery> InstanceTableProxyModel::parseFilterQuery(QString query)
{
    const QRegularExpression pattern = QRegularExpression("(?:(?<prefix>\\S+):)?(?:\"(?<content1>.+)\"|(?<content2>\\S+))");
    QList<InstanceFilterQuery> queries;

    QRegularExpressionMatchIterator it = pattern.globalMatch(query);
    while (it.hasNext()) {
        const QRegularExpressionMatch& match = it.next();
        InstanceList::Column c = InstanceList::NameColumn;

        QString prefix = match.captured("prefix");
        if (prefix.toLower() == "category")
            c = InstanceList::CategoryColumn;
        else if (prefix.toLower() == "version")
            c = InstanceList::GameVersionColumn;

        QString content = match.captured("content1");
        if (content.isNull())
            content = match.captured("content2");

        queries << std::make_pair(c, content.toLower());
    }
    return queries;
}
