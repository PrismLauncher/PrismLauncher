/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceProxyModel.h"

#include "InstanceList.h"
#include "Application.h"
#include <icons/IconList.h>

InstanceProxyModel::InstanceProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
    m_naturalSort.setNumericMode(true);
    m_naturalSort.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    // FIXME: use loaded translation as source of locale instead, hook this up to translation changes
    m_naturalSort.setLocale(QLocale::system());
}

QVariant InstanceProxyModel::data(const QModelIndex & index, int role) const
{
    QVariant data = QSortFilterProxyModel::data(index, role);
    if(role == Qt::DecorationRole)
    {
        if (!data.toString().isEmpty())
            return APPLICATION->icons()->getIcon(data.toString()); //FIXME: Needs QStyledItemDelegate
    }

    switch (index.column()) {
        case InstanceList::LastPlayed: {
            if (role == Qt::DisplayRole) {
                QDateTime foo = data.toDateTime();
                if (foo.isNull() || !foo.isValid() || foo.toMSecsSinceEpoch() == 0)
                    return tr("Never");
            }
            break;
        }
    }
    return data;
}
