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

#include <QCollator>
#include <QSortFilterProxyModel>
#include <optional>

class InstanceProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    InstanceProxyModel(QObject* parent = 0);

    // Shows only instances whose name, Minecraft version or mod loader contains the text
    void setSearchText(const QString& text);
    // Shows only one group; no value shows every group, an empty string the ungrouped instances
    void setGroupFilter(std::optional<QString> group);
    bool isFiltering() const { return !m_searchText.isEmpty() || m_groupFilter.has_value(); }

   protected:
    QVariant data(const QModelIndex& index, int role) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    bool subSortLessThan(const QModelIndex& left, const QModelIndex& right) const;

   private:
    QCollator m_naturalSort;
    QString m_searchText;
    std::optional<QString> m_groupFilter;
};
