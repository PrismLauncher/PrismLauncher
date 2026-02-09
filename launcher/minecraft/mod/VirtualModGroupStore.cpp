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

#include "VirtualModGroupStore.h"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QUuid>

#include "FileSystem.h"

namespace {

[[nodiscard]] QString normalizedManagedPackId(const QString& managedPackId, const QString& fallbackName)
{
    auto id = managedPackId.trimmed();
    if (!id.isEmpty()) {
        return id;
    }

    auto fallback = fallbackName.trimmed();
    if (!fallback.isEmpty()) {
        return fallback;
    }

    return "managed-pack";
}

[[nodiscard]] QString sanitizedIdFragment(QString input)
{
    input = input.trimmed().toLower();
    for (int index = 0; index < input.size(); ++index) {
        auto character = input.at(index);
        bool keep = character.isLetterOrNumber() || character == '-' || character == '_' || character == '.';
        if (!keep) {
            input[index] = '-';
        }
    }

    while (input.contains("--")) {
        input.replace("--", "-");
    }

    input = input.trimmed();
    while (input.startsWith('-')) {
        input.remove(0, 1);
    }
    while (input.endsWith('-')) {
        input.chop(1);
    }

    return input;
}

}  // namespace

VirtualModGroupStore::VirtualModGroupStore(QDir modsDir, QDir indexDir) : m_modsDir(std::move(modsDir)), m_indexDir(std::move(indexDir)) {}

bool VirtualModGroupStore::exists() const
{
    return QFile::exists(storePath());
}

bool VirtualModGroupStore::load()
{
    m_groups.clear();
    m_entries.clear();
    m_legacyNestedFolderMigrationDone = false;

    QFile storeFile(storePath());
    if (!storeFile.exists()) {
        return false;
    }

    if (!storeFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open virtual mod group store:" << storeFile.fileName();
        return false;
    }

    QJsonParseError parseError{};
    auto document = QJsonDocument::fromJson(storeFile.readAll(), &parseError);
    storeFile.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "Could not parse virtual mod group store:" << storeFile.fileName() << parseError.errorString();
        return false;
    }

    if (!fromJson(document.object())) {
        qWarning() << "Virtual mod group store content is invalid:" << storeFile.fileName();
        return false;
    }

    return true;
}

bool VirtualModGroupStore::loadOrCreate()
{
    if (load()) {
        return true;
    }

    m_groups.clear();
    m_entries.clear();
    m_legacyNestedFolderMigrationDone = false;
    return save();
}

bool VirtualModGroupStore::save() const
{
    auto path = storePath();
    if (!FS::ensureFilePathExists(path)) {
        qWarning() << "Could not prepare path for virtual mod group store:" << path;
        return false;
    }

    auto jsonDocument = QJsonDocument(toJson());
    auto serializedDocument = jsonDocument.toJson(QJsonDocument::Indented);

    QFile existingFile(path);
    if (existingFile.exists() && existingFile.open(QIODevice::ReadOnly)) {
        auto currentBytes = existingFile.readAll();
        existingFile.close();
        if (currentBytes == serializedDocument) {
            return true;
        }
    }

    QFile storeFile(path);
    if (!storeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Could not write virtual mod group store:" << path;
        return false;
    }

    auto bytesWritten = storeFile.write(serializedDocument);
    storeFile.flush();
    storeFile.close();
    return bytesWritten == serializedDocument.size();
}

QList<VirtualModGroupStore::Group> VirtualModGroupStore::groups() const
{
    return m_groups.values();
}

QList<VirtualModGroupStore::Entry> VirtualModGroupStore::entries() const
{
    return m_entries.values();
}

std::optional<VirtualModGroupStore::Entry> VirtualModGroupStore::entry(QString fileKey) const
{
    fileKey = fileKeyForFileName(std::move(fileKey));
    auto entryIter = m_entries.constFind(fileKey);
    if (entryIter == m_entries.constEnd()) {
        return std::nullopt;
    }

    return entryIter.value();
}

bool VirtualModGroupStore::hasEntry(QString fileKey) const
{
    fileKey = fileKeyForFileName(std::move(fileKey));
    return m_entries.contains(fileKey);
}

void VirtualModGroupStore::upsertEntry(Entry entry)
{
    entry.fileKey = fileKeyForFileName(std::move(entry.fileKey));
    if (entry.fileName.trimmed().isEmpty()) {
        entry.fileName = entry.fileKey;
    }
    if (!entry.groupId.isEmpty() && !groupExists(entry.groupId)) {
        entry.groupId.clear();
    }
    m_entries.insert(entry.fileKey, std::move(entry));
}

bool VirtualModGroupStore::removeEntry(const QString& fileKey)
{
    return m_entries.remove(fileKeyForFileName(fileKey)) > 0;
}

void VirtualModGroupStore::removeEntriesNotIn(const QSet<QString>& fileKeys)
{
    QSet<QString> normalizedKeys;
    normalizedKeys.reserve(fileKeys.size());
    for (auto const& fileKey : fileKeys) {
        normalizedKeys.insert(fileKeyForFileName(fileKey));
    }

    for (auto entryIter = m_entries.begin(); entryIter != m_entries.end();) {
        if (!normalizedKeys.contains(entryIter.key())) {
            entryIter = m_entries.erase(entryIter);
        } else {
            ++entryIter;
        }
    }
}

bool VirtualModGroupStore::groupExists(const QString& groupId) const
{
    if (groupId.isEmpty()) {
        return true;
    }
    return m_groups.contains(groupId);
}

QString VirtualModGroupStore::createGroup(QString name, QString parentId)
{
    Q_UNUSED(parentId)
    parentId.clear();

    name = ensureUniqueGroupName(name.trimmed(), parentId);
    if (name.isEmpty()) {
        return {};
    }

    Group group;
    group.id = generateGroupId(name);
    group.name = name;
    group.parentId = parentId;

    m_groups.insert(group.id, group);
    return group.id;
}

bool VirtualModGroupStore::renameGroup(const QString& groupId, const QString& newName)
{
    auto groupIter = m_groups.find(groupId);
    if (groupIter == m_groups.end()) {
        return false;
    }

    auto normalizedName = ensureUniqueGroupName(newName.trimmed(), groupIter->parentId);
    if (normalizedName.isEmpty()) {
        return false;
    }

    groupIter->name = normalizedName;
    return true;
}

bool VirtualModGroupStore::moveGroup(const QString& groupId, const QString& newParentId)
{
    auto groupIter = m_groups.find(groupId);
    if (groupIter == m_groups.end()) {
        return false;
    }

    Q_UNUSED(newParentId)
    QString normalizedParent;

    if (wouldCreateGroupCycle(groupId, normalizedParent)) {
        return false;
    }

    groupIter->parentId = normalizedParent;
    groupIter->name = ensureUniqueGroupName(groupIter->name, normalizedParent);
    return true;
}

bool VirtualModGroupStore::deleteGroup(const QString& groupId)
{
    if (!m_groups.contains(groupId)) {
        return false;
    }

    auto groupIdsToDelete = groupSubtreeIds(groupId);
    QSet<QString> groupDeleteSet(groupIdsToDelete.begin(), groupIdsToDelete.end());

    for (auto entryIter = m_entries.begin(); entryIter != m_entries.end(); ++entryIter) {
        if (groupDeleteSet.contains(entryIter->groupId)) {
            entryIter->groupId.clear();
        }
    }

    for (auto const& groupIdToDelete : groupIdsToDelete) {
        m_groups.remove(groupIdToDelete);
    }

    return true;
}

bool VirtualModGroupStore::assignEntryToGroup(const QString& fileKey, const QString& groupId)
{
    auto normalizedKey = fileKeyForFileName(fileKey);
    auto entryIter = m_entries.find(normalizedKey);
    if (entryIter == m_entries.end()) {
        return false;
    }

    if (!groupExists(groupId)) {
        return false;
    }

    entryIter->groupId = groupId;
    return true;
}

bool VirtualModGroupStore::assignEntriesToGroup(const QStringList& fileKeys, const QString& groupId)
{
    bool updatedAny = false;
    for (auto const& fileKey : fileKeys) {
        if (assignEntryToGroup(fileKey, groupId)) {
            updatedAny = true;
        }
    }
    return updatedAny;
}

bool VirtualModGroupStore::isEntryInGroupSubtree(const QString& fileKey, const QString& groupId) const
{
    if (groupId.isEmpty()) {
        return true;
    }

    auto entryOpt = entry(fileKey);
    if (!entryOpt.has_value()) {
        return false;
    }

    auto subtreeIds = groupSubtreeIds(groupId);
    return subtreeIds.contains(entryOpt->groupId);
}

QList<QString> VirtualModGroupStore::groupSubtreeIds(const QString& groupId) const
{
    if (groupId.isEmpty() || !m_groups.contains(groupId)) {
        return {};
    }

    QList<QString> subtree;
    subtree.push_back(groupId);

    int index = 0;
    while (index < subtree.size()) {
        auto currentId = subtree.at(index);
        for (auto const& group : m_groups) {
            if (group.parentId == currentId && !subtree.contains(group.id)) {
                subtree.push_back(group.id);
            }
        }
        ++index;
    }

    return subtree;
}

QString VirtualModGroupStore::ensureManagedPackGroup(QString managedPackType, QString managedPackId, QString fallbackName)
{
    managedPackType = managedPackType.trimmed();
    managedPackId = normalizedManagedPackId(managedPackId, fallbackName);

    auto existingId = findManagedPackGroup(managedPackType, managedPackId);
    if (!existingId.isEmpty()) {
        return existingId;
    }

    Group group;
    group.kind = GroupKind::MANAGED_PACK;
    group.managedPackType = managedPackType;
    group.managedPackId = managedPackId;
    group.name = fallbackName.trimmed().isEmpty() ? QObject::tr("Managed Pack Mods") : fallbackName.trimmed();

    auto typeFragment = sanitizedIdFragment(managedPackType);
    auto idFragment = sanitizedIdFragment(managedPackId);
    if (typeFragment.isEmpty()) {
        typeFragment = "managed";
    }
    if (idFragment.isEmpty()) {
        idFragment = "pack";
    }

    group.id = QString("managed-pack-%1-%2").arg(typeFragment, idFragment);
    if (m_groups.contains(group.id)) {
        group.id = generateGroupId(group.name);
    }

    m_groups.insert(group.id, group);
    return group.id;
}

QString VirtualModGroupStore::findManagedPackGroup(const QString& managedPackType, const QString& managedPackId) const
{
    auto normalizedType = managedPackType.trimmed();
    auto normalizedId = managedPackId.trimmed();

    for (auto const& group : m_groups) {
        if (group.kind != GroupKind::MANAGED_PACK) {
            continue;
        }
        if (group.managedPackType.compare(normalizedType, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (group.managedPackId.compare(normalizedId, Qt::CaseInsensitive) == 0) {
            return group.id;
        }
    }

    return {};
}

QList<VirtualModGroupStore::GroupDisplay> VirtualModGroupStore::groupDisplayList() const
{
    QList<GroupDisplay> groups;
    appendDisplayGroups({}, 0, groups);
    return groups;
}

QString VirtualModGroupStore::fileKeyForFileName(QString fileName)
{
    if (fileName.endsWith(".disabled", Qt::CaseInsensitive)) {
        fileName.chop(9);
    }
    return fileName;
}

QString VirtualModGroupStore::groupKindName(GroupKind groupKind)
{
    switch (groupKind) {
        case GroupKind::CUSTOM:
            return "custom";
        case GroupKind::MANAGED_PACK:
            return "managed-pack";
    }
    return "custom";
}

VirtualModGroupStore::GroupKind VirtualModGroupStore::groupKindFromName(const QString& groupKind)
{
    if (groupKind.compare("managed-pack", Qt::CaseInsensitive) == 0) {
        return GroupKind::MANAGED_PACK;
    }
    return GroupKind::CUSTOM;
}

QString VirtualModGroupStore::sourceTypeName(SourceType sourceType)
{
    switch (sourceType) {
        case SourceType::LOCAL_NO_SOURCE:
            return "local-no-source";
        case SourceType::PROVIDER_LINKED:
            return "provider-linked";
        case SourceType::MANAGED_PACK:
            return "managed-pack";
    }
    return "local-no-source";
}

VirtualModGroupStore::SourceType VirtualModGroupStore::sourceTypeFromName(const QString& sourceType)
{
    if (sourceType.compare("provider-linked", Qt::CaseInsensitive) == 0) {
        return SourceType::PROVIDER_LINKED;
    }
    if (sourceType.compare("managed-pack", Qt::CaseInsensitive) == 0) {
        return SourceType::MANAGED_PACK;
    }
    return SourceType::LOCAL_NO_SOURCE;
}

QString VirtualModGroupStore::storePath() const
{
    return m_indexDir.absoluteFilePath(STORE_FILE_NAME);
}

bool VirtualModGroupStore::fromJson(const QJsonObject& rootObject)
{
    auto formatVersion = rootObject.value("formatVersion").toInt(-1);
    if (formatVersion != FORMAT_VERSION) {
        return false;
    }

    auto legacyObject = rootObject.value("migrations");
    if (legacyObject.isObject()) {
        m_legacyNestedFolderMigrationDone = legacyObject.toObject().value("legacyNestedFoldersImported").toBool(false);
    } else {
        m_legacyNestedFolderMigrationDone = rootObject.value("legacyNestedFoldersImported").toBool(false);
    }

    auto groupsArrayValue = rootObject.value("groups");
    auto entriesArrayValue = rootObject.value("entries");
    if (!groupsArrayValue.isArray() || !entriesArrayValue.isArray()) {
        return false;
    }

    auto groupsArray = groupsArrayValue.toArray();
    for (auto const& groupValue : groupsArray) {
        if (!groupValue.isObject()) {
            continue;
        }

        auto groupObject = groupValue.toObject();
        Group group;
        group.id = groupObject.value("id").toString().trimmed();
        group.name = groupObject.value("name").toString().trimmed();
        group.parentId.clear();
        group.name = ensureUniqueGroupName(group.name, {});
        group.kind = groupKindFromName(groupObject.value("kind").toString());
        group.managedPackType = groupObject.value("managedPackType").toString().trimmed();
        group.managedPackId = groupObject.value("managedPackId").toString().trimmed();

        if (group.id.isEmpty() || group.name.isEmpty()) {
            continue;
        }

        m_groups.insert(group.id, group);
    }

    auto entriesArray = entriesArrayValue.toArray();
    for (auto const& entryValue : entriesArray) {
        if (!entryValue.isObject()) {
            continue;
        }

        auto entryObject = entryValue.toObject();
        Entry entry;
        entry.fileKey = fileKeyForFileName(entryObject.value("fileKey").toString().trimmed());
        entry.fileName = entryObject.value("fileName").toString().trimmed();
        entry.groupId = entryObject.value("groupId").toString().trimmed();
        entry.sourceType = sourceTypeFromName(entryObject.value("sourceType").toString());
        entry.linkedMetadataSlug = entryObject.value("linkedMetadataSlug").toString().trimmed();
        entry.provider = entryObject.value("provider").toString().trimmed();
        entry.projectId = entryObject.value("projectId").toString().trimmed();

        if (entry.fileKey.isEmpty()) {
            continue;
        }

        if (entry.fileName.isEmpty()) {
            entry.fileName = entry.fileKey;
        }

        if (!entry.groupId.isEmpty() && !m_groups.contains(entry.groupId)) {
            entry.groupId.clear();
        }

        m_entries.insert(entry.fileKey, entry);
    }

    removeOrphanedGroups();
    return true;
}

QJsonObject VirtualModGroupStore::toJson() const
{
    QJsonObject rootObject;
    rootObject.insert("formatVersion", FORMAT_VERSION);

    QJsonObject migrationsObject;
    migrationsObject.insert("legacyNestedFoldersImported", m_legacyNestedFolderMigrationDone);
    rootObject.insert("migrations", migrationsObject);

    QJsonArray groupsArray;
    auto groupsList = groups();
    std::sort(groupsList.begin(), groupsList.end(), [](const Group& left, const Group& right) { return left.id < right.id; });

    for (auto const& group : groupsList) {
        QJsonObject groupObject;
        groupObject.insert("id", group.id);
        groupObject.insert("name", group.name);
        if (group.parentId.isEmpty()) {
            groupObject.insert("parentId", QJsonValue(QJsonValue::Null));
        } else {
            groupObject.insert("parentId", group.parentId);
        }
        groupObject.insert("kind", groupKindName(group.kind));
        if (!group.managedPackType.isEmpty()) {
            groupObject.insert("managedPackType", group.managedPackType);
        }
        if (!group.managedPackId.isEmpty()) {
            groupObject.insert("managedPackId", group.managedPackId);
        }
        groupsArray.append(groupObject);
    }
    rootObject.insert("groups", groupsArray);

    QJsonArray entriesArray;
    auto entriesList = entries();
    std::sort(entriesList.begin(), entriesList.end(), [](const Entry& left, const Entry& right) { return left.fileKey < right.fileKey; });
    for (auto const& entry : entriesList) {
        QJsonObject entryObject;
        entryObject.insert("fileKey", entry.fileKey);
        entryObject.insert("fileName", entry.fileName);
        if (entry.groupId.isEmpty()) {
            entryObject.insert("groupId", QJsonValue(QJsonValue::Null));
        } else {
            entryObject.insert("groupId", entry.groupId);
        }
        entryObject.insert("sourceType", sourceTypeName(entry.sourceType));
        if (!entry.linkedMetadataSlug.isEmpty()) {
            entryObject.insert("linkedMetadataSlug", entry.linkedMetadataSlug);
        }
        if (!entry.provider.isEmpty()) {
            entryObject.insert("provider", entry.provider);
        }
        if (!entry.projectId.isEmpty()) {
            entryObject.insert("projectId", entry.projectId);
        }
        entriesArray.append(entryObject);
    }
    rootObject.insert("entries", entriesArray);

    return rootObject;
}

void VirtualModGroupStore::removeOrphanedGroups()
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto groupIter = m_groups.begin(); groupIter != m_groups.end(); ++groupIter) {
            if (groupIter->parentId.isEmpty()) {
                continue;
            }
            if (!m_groups.contains(groupIter->parentId) || groupIter->parentId == groupIter->id) {
                groupIter->parentId.clear();
                changed = true;
            }
        }
    }
}

bool VirtualModGroupStore::wouldCreateGroupCycle(const QString& groupId, const QString& newParentId) const
{
    if (groupId == newParentId) {
        return true;
    }

    auto currentParent = newParentId;
    while (!currentParent.isEmpty()) {
        if (currentParent == groupId) {
            return true;
        }
        auto parentIter = m_groups.constFind(currentParent);
        if (parentIter == m_groups.constEnd()) {
            break;
        }
        currentParent = parentIter->parentId;
    }

    return false;
}

QString VirtualModGroupStore::generateGroupId(const QString& name) const
{
    auto idFragment = sanitizedIdFragment(name);
    if (idFragment.isEmpty()) {
        idFragment = "group";
    }

    QString candidate = "group-" + idFragment;
    if (!m_groups.contains(candidate)) {
        return candidate;
    }

    int suffix = 2;
    while (m_groups.contains(candidate + "-" + QString::number(suffix))) {
        ++suffix;
    }

    return candidate + "-" + QString::number(suffix);
}

QString VirtualModGroupStore::ensureUniqueGroupName(const QString& name, const QString& parentId) const
{
    auto normalized = name.trimmed();
    if (normalized.isEmpty()) {
        normalized = QObject::tr("New Group");
    }

    auto hasSiblingWithName = [this, &parentId](const QString& groupName) {
        for (auto const& group : m_groups) {
            if (group.parentId.compare(parentId, Qt::CaseSensitive) == 0 && group.name.compare(groupName, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

    if (!hasSiblingWithName(normalized)) {
        return normalized;
    }

    int suffix = 2;
    QString candidate;
    do {
        candidate = QString("%1 (%2)").arg(normalized).arg(suffix);
        ++suffix;
    } while (hasSiblingWithName(candidate));

    return candidate;
}

void VirtualModGroupStore::appendDisplayGroups(const QString& parentId, int depth, QList<GroupDisplay>& out) const
{
    QList<Group> children;
    children.reserve(m_groups.size());
    for (auto const& group : m_groups) {
        if (group.parentId == parentId) {
            children.push_back(group);
        }
    }

    std::sort(children.begin(), children.end(),
              [](const Group& left, const Group& right) { return left.name.localeAwareCompare(right.name) < 0; });

    for (auto const& group : children) {
        out.push_back({ group.id, group.name, depth });
        appendDisplayGroups(group.id, depth + 1, out);
    }
}
