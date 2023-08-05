/* Copyright 2015-2021 MultiMC Contributors
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

#include <QAbstractListModel>
#include <memory>

#include "BaseEntity.h"
#include "meta/VersionList.h"

class Task;

namespace Meta {

class Index : public QAbstractListModel, public BaseEntity {
    Q_OBJECT
   public:
    explicit Index(QObject* parent = nullptr);
    explicit Index(const QVector<VersionList::Ptr>& lists, QObject* parent = nullptr);

    enum { UidRole = Qt::UserRole, NameRole, ListPtrRole };

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QString localFilename() const override { return "index.json"; }

    // queries
    VersionList::Ptr get(const QString& uid);
    Version::Ptr get(const QString& uid, const QString& version);
    bool hasUid(const QString& uid) const;

    QVector<VersionList::Ptr> lists() const { return m_lists; }

   public:  // for usage by parsers only
    void merge(const std::shared_ptr<Index>& other);
    void parse(const QJsonObject& obj) override;

   private:
    QVector<VersionList::Ptr> m_lists;
    QHash<QString, VersionList::Ptr> m_uids;

    void connectVersionList(const int row, const VersionList::Ptr& list);
};
}  // namespace Meta
