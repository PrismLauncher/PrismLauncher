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

#include "ModGroupStore.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QUuid>

#include <algorithm>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"

ModGroupStore::ModGroupStore(QDir modsDir)
    : m_indexDir(modsDir.filePath(".index"))
    , m_filePath(m_indexDir.filePath(QString("%1-groups.json").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME)))
{
    load();
}

QString ModGroupStore::createGroup(const QString& name)
{
    auto groupName = name.trimmed();
    if (groupName.isEmpty())
        return {};

    Group group;
    do {
        group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    } while (hasGroup(group.id));

    group.name = groupName;
    m_groups.append(group);

    if (!save()) {
        m_groups.removeLast();
        return {};
    }

    return group.id;
}

bool ModGroupStore::deleteGroup(const QString& groupId)
{
    auto iter = std::find_if(m_groups.begin(), m_groups.end(), [&groupId](const Group& group) { return group.id == groupId; });
    if (iter == m_groups.end())
        return false;

    const auto previousGroups = m_groups;
    const auto previousAssignments = m_assignments;

    m_groups.erase(iter);

    for (auto assignment = m_assignments.begin(); assignment != m_assignments.end(); ++assignment) {
        if (assignment.value() == groupId)
            assignment.value().clear();
    }

    if (save())
        return true;

    m_groups = previousGroups;
    m_assignments = previousAssignments;
    return false;
}

bool ModGroupStore::assign(const QString& fileKey, const std::optional<QString>& groupId)
{
    auto normalizedFileKey = normalizeFileKey(fileKey);
    if (normalizedFileKey.isEmpty())
        return false;

    QString normalizedGroupId;
    if (groupId.has_value()) {
        normalizedGroupId = groupId->trimmed();
        if (!normalizedGroupId.isEmpty() && !hasGroup(normalizedGroupId))
            return false;
    }

    if (m_assignments.contains(normalizedFileKey) && m_assignments.value(normalizedFileKey) == normalizedGroupId)
        return true;

    const bool hadPreviousValue = m_assignments.contains(normalizedFileKey);
    const auto previousValue = m_assignments.value(normalizedFileKey);

    m_assignments[normalizedFileKey] = normalizedGroupId;
    if (save())
        return true;

    if (hadPreviousValue) {
        m_assignments[normalizedFileKey] = previousValue;
    } else {
        m_assignments.remove(normalizedFileKey);
    }
    return false;
}

std::optional<QString> ModGroupStore::groupFor(const QString& fileKey) const
{
    auto normalizedFileKey = normalizeFileKey(fileKey);
    if (!m_assignments.contains(normalizedFileKey))
        return std::nullopt;

    const auto value = m_assignments.value(normalizedFileKey);
    if (value.isEmpty())
        return std::nullopt;

    return value;
}

bool ModGroupStore::syncWithFilesystem(const QStringList& fileKeys)
{
    QSet<QString> filesOnDisk;
    filesOnDisk.reserve(fileKeys.size());

    for (const auto& fileKey : fileKeys) {
        auto normalizedFileKey = normalizeFileKey(fileKey);
        if (!normalizedFileKey.isEmpty())
            filesOnDisk.insert(normalizedFileKey);
    }

    bool changed = false;
    const auto previousAssignments = m_assignments;

    for (const auto& fileKey : filesOnDisk) {
        if (!m_assignments.contains(fileKey)) {
            m_assignments[fileKey] = {};
            changed = true;
        }
    }

    for (auto assignment = m_assignments.begin(); assignment != m_assignments.end();) {
        if (!filesOnDisk.contains(assignment.key())) {
            assignment = m_assignments.erase(assignment);
            changed = true;
            continue;
        }

        ++assignment;
    }

    if (!changed)
        return true;

    if (save())
        return true;

    m_assignments = previousAssignments;
    return false;
}

bool ModGroupStore::save()
{
    QFileInfo groupFile(m_filePath);
    bool hasGroupedResources =
        std::any_of(m_assignments.begin(), m_assignments.end(), [](const QString& groupId) { return !groupId.isEmpty(); });
    if (!groupFile.exists() && m_groups.isEmpty() && !hasGroupedResources)
        return true;

    if (!FS::ensureFolderPathExists(m_indexDir.absolutePath()))
        return false;

    try {
        Json::write(serialize(), m_filePath);
        return true;
    } catch (const FS::FileSystemException& e) {
        qWarning() << "Could not save mod group metadata file:" << e.cause();
        return false;
    }
}

QString ModGroupStore::normalizeFileKey(QString fileKey)
{
    if (fileKey.endsWith(".disabled"))
        fileKey.chop(9);

    return fileKey;
}

bool ModGroupStore::load()
{
    m_groups.clear();
    m_assignments.clear();

    QFileInfo groupFile(m_filePath);
    if (!groupFile.exists())
        return true;

    QJsonParseError parseError;
    QJsonDocument document;

    try {
        document = QJsonDocument::fromJson(FS::read(m_filePath), &parseError);
    } catch (const FS::FileSystemException& e) {
        qWarning() << "Could not read mod group metadata file:" << e.cause();
        return false;
    }

    if (parseError.error != QJsonParseError::NoError) {
        qWarning()
            << QString("Could not parse mod group metadata file: %1 at offset %2").arg(parseError.errorString()).arg(parseError.offset);
        return false;
    }

    if (!document.isObject()) {
        qWarning() << "Invalid mod group metadata file: root must be a JSON object.";
        return false;
    }

    return deserialize(document.object());
}

bool ModGroupStore::deserialize(const QJsonObject& root)
{
    int formatVersion = root.value("formatVersion").toInt(-1);
    if (formatVersion != s_formatVersion) {
        qWarning() << "Invalid mod group metadata format version:" << formatVersion;
        return false;
    }

    if (!root.value("groups").isArray()) {
        qWarning() << "Invalid mod group metadata file: 'groups' must be an array.";
        return false;
    }

    auto groupsValue = root.value("groups").toArray();
    for (const auto& groupValue : groupsValue) {
        if (!groupValue.isObject()) {
            qWarning() << "Invalid mod group metadata file: all group entries must be objects.";
            continue;
        }

        auto groupObject = groupValue.toObject();
        auto groupId = groupObject.value("id");
        auto groupName = groupObject.value("name");

        if (!groupId.isString() || !groupName.isString()) {
            qWarning() << "Invalid mod group metadata file: group entries must contain string 'id' and 'name'.";
            continue;
        }

        const auto parsedGroupId = groupId.toString().trimmed();
        if (parsedGroupId.isEmpty())
            continue;

        if (hasGroup(parsedGroupId))
            continue;

        m_groups.append({ parsedGroupId, groupName.toString() });
    }

    if (!root.value("assignments").isObject()) {
        qWarning() << "Invalid mod group metadata file: 'assignments' must be an object.";
        return false;
    }

    QSet<QString> groupIds;
    groupIds.reserve(m_groups.size());
    for (const auto& group : m_groups)
        groupIds.insert(group.id);

    auto assignmentsObject = root.value("assignments").toObject();
    for (auto assignment = assignmentsObject.begin(); assignment != assignmentsObject.end(); ++assignment) {
        auto normalizedFileKey = normalizeFileKey(assignment.key());
        if (normalizedFileKey.isEmpty())
            continue;

        if (assignment.value().isNull()) {
            m_assignments[normalizedFileKey] = {};
            continue;
        }

        if (!assignment.value().isString()) {
            qWarning() << "Invalid mod group metadata file: assignment values must be a group id string or null.";
            continue;
        }

        auto groupId = assignment.value().toString().trimmed();
        if (groupId.isEmpty() || !groupIds.contains(groupId)) {
            m_assignments[normalizedFileKey] = {};
            continue;
        }

        m_assignments[normalizedFileKey] = groupId;
    }

    return true;
}

QJsonObject ModGroupStore::serialize() const
{
    QJsonObject root;
    root.insert("formatVersion", s_formatVersion);

    QJsonArray groupsArray;
    groupsArray.reserve(m_groups.size());

    for (const auto& group : m_groups) {
        QJsonObject groupObject;
        groupObject.insert("id", group.id);
        groupObject.insert("name", group.name);
        groupsArray.append(groupObject);
    }

    root.insert("groups", groupsArray);

    QJsonObject assignmentsObject;
    auto sortedKeys = m_assignments.keys();
    std::sort(sortedKeys.begin(), sortedKeys.end(), [](const QString& a, const QString& b) { return a < b; });

    for (const auto& fileKey : sortedKeys) {
        const auto groupId = m_assignments.value(fileKey);
        assignmentsObject.insert(fileKey, groupId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(groupId));
    }

    root.insert("assignments", assignmentsObject);

    return root;
}

bool ModGroupStore::hasGroup(const QString& groupId) const
{
    return std::any_of(m_groups.begin(), m_groups.end(), [&groupId](const Group& group) { return group.id == groupId; });
}
