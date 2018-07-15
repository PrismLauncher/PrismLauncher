/* Copyright 2013-2018 MultiMC Contributors
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

#include "GroupedProxyModel.h"

#include "GroupView.h"
#include <QDebug>

GroupedProxyModel::GroupedProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool GroupedProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QString leftCategory = left.data(GroupViewRoles::GroupRole).toString();
    const QString rightCategory = right.data(GroupViewRoles::GroupRole).toString();
    if (leftCategory == rightCategory)
    {
        return subSortLessThan(left, right);
    }
    else
    {
        // FIXME: real group sorting happens in GroupView::updateGeometries(), see LocaleString
        auto result = leftCategory.localeAwareCompare(rightCategory);
        if(result == 0)
        {
            return subSortLessThan(left, right);
        }
        return result < 0;
    }
}

bool GroupedProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return left.row() < right.row();
}
