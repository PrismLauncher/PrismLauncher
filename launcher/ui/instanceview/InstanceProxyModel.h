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
#include "modplatform/ModIndex.h"

class InstanceProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    explicit InstanceProxyModel(QObject* parent = nullptr);

    void sortBy(QStringList mcVersions, ModPlatform::ModLoaderTypes loader = ModPlatform::ModLoaderType::None);
    void setSearchTerm(QString searchTerm);

   protected:
    QVariant data(const QModelIndex& index, int role) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    bool subSortLessThan(const QModelIndex& left, const QModelIndex& right) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

   private:
    QCollator m_naturalSort;

    QString m_searchTerm;
    QStringList m_mcVersions;
    ModPlatform::ModLoaderTypes m_loader;
};
