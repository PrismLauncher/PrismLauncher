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

#pragma once

#include "InstanceList.h"

#include <QCollator>
#include <QSortFilterProxyModel>

typedef std::pair<InstanceList::Column, QString> InstanceFilterQuery;

class InstanceTableProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    InstanceTableProxyModel(QObject* parent = 0);

    void setFilterQuery(const QString query);
    void setFilterQuery(const QList<InstanceFilterQuery> query);

    static QList<InstanceFilterQuery> parseFilterQuery(const QString query);

   protected:
    QVariant data(const QModelIndex& index, int role) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

   private:
    QCollator m_naturalSort;
    QList<InstanceFilterQuery> m_filter;
};
