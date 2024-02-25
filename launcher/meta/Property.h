// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 初夏同学 <2411829240@qq.com>
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
    void downloadAndApplyProperties();

   signals:
    void succeededApplyProperties(QHash<QString, QString> succeed);
    void failedApplyProperties(QString reasons);

   public:  // for usage by parsers only
    void configurate(const std::shared_ptr<Property>& other);
    void parse(const QJsonObject& obj) override;

   private:
    inline void apply();

   private:
    QHash<QString, QString> m_properties;
};
}  // namespace Meta
