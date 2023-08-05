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

#include <QJsonObject>
#include <memory>
#include "BaseEntity.h"
#include "BaseVersionList.h"

#include "meta/Version.h"

namespace Meta {

class VersionList : public BaseVersionList, public BaseEntity {
    Q_OBJECT
    Q_PROPERTY(QString uid READ uid CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
   public:
    explicit VersionList(const QString& uid, QObject* parent = nullptr);

    using Ptr = std::shared_ptr<VersionList>;

    enum Roles { UidRole = Qt::UserRole + 100, TimeRole, RequiresRole, VersionPtrRole };

    Task::Ptr getLoadTask() override;
    bool isLoaded() override;
    const BaseVersion::Ptr at(int i) const override;
    int count() const override;
    void sortVersions() override;

    BaseVersion::Ptr getRecommended() const override;

    QVariant data(const QModelIndex& index, int role) const override;
    RoleList providesRoles() const override;
    QHash<int, QByteArray> roleNames() const override;

    QString localFilename() const override;

    QString uid() const { return m_uid; }
    QString name() const { return m_name; }
    QString humanReadable() const;

    Version::Ptr getVersion(const QString& version);
    bool hasVersion(QString version) const;

    QVector<Version::Ptr> versions() const { return m_versions; }

   public:  // for usage only by parsers
    void setName(const QString& name);
    void setVersions(const QVector<Version::Ptr>& versions);
    void merge(const VersionList::Ptr& other);
    void mergeFromIndex(const VersionList::Ptr& other);
    void parse(const QJsonObject& obj) override;

   signals:
    void nameChanged(const QString& name);

   protected slots:
    void updateListData(QList<BaseVersion::Ptr>) override {}

   private:
    QVector<Version::Ptr> m_versions;
    QHash<QString, Version::Ptr> m_lookup;
    QString m_uid;
    QString m_name;

    Version::Ptr m_recommended;

    void setupAddedVersion(const int row, const Version::Ptr& version);
};
}  // namespace Meta
Q_DECLARE_METATYPE(Meta::VersionList::Ptr)
