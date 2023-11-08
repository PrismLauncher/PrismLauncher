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

class Property : public QObject, public BaseEntity {
    Q_OBJECT
   public:
    explicit Property(QObject* parent = nullptr);
    explicit Property(const QHash<QString, QString>& properties, QObject* parent = nullptr);

    QString localFilename() const override { return "property.json"; }

    // Properties
    void applyProperties();

   signals:
    void succeededApplyProperties(QHash<QString, QString> succeed);

   public:  // for usage by parsers only
    void configurate(const std::shared_ptr<Property>& other);
    void parse(const QJsonObject& obj) override;

   private:
    inline void apply();

   private:
    QHash<QString, QString> m_properties;
};
}  // namespace Meta
