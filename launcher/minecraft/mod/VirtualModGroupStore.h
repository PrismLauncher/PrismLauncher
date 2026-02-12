// SPDX-FileCopyrightText: 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
//
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

#include <optional>

#include <QDir>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

class QJsonObject;

class VirtualModGroupStore {
   public:
    static inline constexpr auto FORMAT_VERSION = 1;

    enum class GroupKind {
        CUSTOM,
        MANAGED_PACK,
    };

    enum class SourceType {
        LOCAL_NO_SOURCE,
        PROVIDER_LINKED,
        MANAGED_PACK,
    };

    struct Group {
        QString id;
        QString name;
        GroupKind kind = GroupKind::CUSTOM;
        QString managedPackType;
        QString managedPackId;
    };

    struct Entry {
        QString fileKey;
        QString fileName;
        QString groupId;
        SourceType sourceType = SourceType::LOCAL_NO_SOURCE;
        QString linkedMetadataSlug;
        QString provider;
        QString projectId;
    };

    explicit VirtualModGroupStore(QDir indexDir);

    [[nodiscard]] bool exists() const;
    [[nodiscard]] bool load();
    [[nodiscard]] bool loadOrCreate();
    [[nodiscard]] bool save() const;
    [[nodiscard]] static QString storeFileName();

    [[nodiscard]] QList<Group> groups() const;
    [[nodiscard]] QList<Entry> entries() const;
    [[nodiscard]] const QHash<QString, Entry>& entriesByKey() const { return m_entries; }
    [[nodiscard]] std::optional<Entry> entry(QString fileKey) const;
    [[nodiscard]] bool hasEntry(QString fileKey) const;
    void upsertEntry(Entry entry);
    [[nodiscard]] bool removeEntry(const QString& fileKey);
    void removeEntriesNotIn(const QSet<QString>& fileKeys);

    [[nodiscard]] bool groupExists(const QString& groupId) const;
    [[nodiscard]] QString createGroup(QString name);
    [[nodiscard]] bool renameGroup(const QString& groupId, const QString& newName);
    [[nodiscard]] bool deleteGroup(const QString& groupId);

    [[nodiscard]] bool assignEntryToGroup(const QString& fileKey, const QString& groupId);
    [[nodiscard]] bool assignEntriesToGroup(const QStringList& fileKeys, const QString& groupId);
    [[nodiscard]] bool isEntryInGroup(const QString& fileKey, const QString& groupId) const;

    [[nodiscard]] QString ensureManagedPackGroup(QString managedPackType, QString managedPackId, QString fallbackName);
    [[nodiscard]] QString findManagedPackGroup(const QString& managedPackType, const QString& managedPackId) const;

    [[nodiscard]] static QString fileKeyForFileName(QString fileName);

   private:
    [[nodiscard]] QString storePath() const;

    [[nodiscard]] bool fromJson(const QJsonObject& rootObject);
    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] QString generateGroupId(const QString& name) const;
    [[nodiscard]] QString ensureUniqueGroupName(const QString& name) const;

   private:
    QDir m_indexDir;
    QHash<QString, Group> m_groups;
    QHash<QString, Entry> m_entries;
};
