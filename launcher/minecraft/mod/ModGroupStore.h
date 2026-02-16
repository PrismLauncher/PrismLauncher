// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
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

#include <QDir>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class ModGroupStore {
   public:
    struct Group {
        QString id;
        QString name;
    };

    explicit ModGroupStore(QDir modsDir);

    QString createGroup(const QString& name);
    bool deleteGroup(const QString& groupId);

    bool assign(const QString& fileKey, const QString& groupId = {});
    QString groupFor(const QString& fileKey) const;
    QList<Group> groups() const { return m_groups; }
    static QString normalizeFileKey(QString fileKey);

    bool syncWithFilesystem(const QStringList& fileKeys);
    bool save();

   private:
    static constexpr int s_formatVersion = 1;

    bool load();
    bool deserialize(const QJsonObject& root);
    QJsonObject serialize() const;
    bool hasGroup(const QString& groupId) const;

   private:
    QDir m_indexDir;
    QString m_filePath;

    QList<Group> m_groups;
    QHash<QString, QString> m_assignments;
};
