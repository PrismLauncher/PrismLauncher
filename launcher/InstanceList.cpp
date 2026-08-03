// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceList.h"

#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>
#include <QSet>
#include <QStack>
#include <QTimer>
#include <QUuid>
#include <algorithm>

#include "BaseInstance.h"
#include "ExponentialSeries.h"
#include "FileSystem.h"

#include "InstanceTask.h"
#include "NullInstance.h"
#include "WatchLock.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/INISettingsObject.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

const static int g_GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(SettingsObject* settings, const QStringList& instDirs, QObject* parent)
    : QAbstractListModel(parent), m_globalSettings(settings)
{
    resumeWatch();

    connect(this, &InstanceList::instancesChanged, this, &InstanceList::providerUpdated);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &InstanceList::instanceDirContentsChanged);

    for (const auto& dir : instDirs) {
        // Create and normalize path
        QDir::current().mkpath(dir);
        // NOTE: canonicalPath requires the path to exist. Do not move this above the creation block!
        QString canonical = QDir(dir).canonicalPath();
        if (!canonical.isEmpty() && !m_instDirs.contains(canonical)) {
            m_instDirs << canonical;
            m_watcher->addPath(canonical);
        }
    }
}

Qt::DropActions InstanceList::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions InstanceList::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool InstanceList::canDropMimeData(const QMimeData* data,
                                   [[maybe_unused]] Qt::DropAction action,
                                   [[maybe_unused]] int row,
                                   [[maybe_unused]] int column,
                                   [[maybe_unused]] const QModelIndex& parent) const
{
    return data != nullptr && data->hasFormat("application/x-instanceid");
}

bool InstanceList::dropMimeData(const QMimeData* data,
                                [[maybe_unused]] Qt::DropAction action,
                                [[maybe_unused]] int row,
                                [[maybe_unused]] int column,
                                [[maybe_unused]] const QModelIndex& parent)
{
    return data != nullptr && data->hasFormat("application/x-instanceid");
}

QStringList InstanceList::mimeTypes() const
{
    auto types = QAbstractListModel::mimeTypes();
    types.push_back("application/x-instanceid");
    return types;
}

QMimeData* InstanceList::mimeData(const QModelIndexList& indexes) const
{
    auto* mimeData = QAbstractListModel::mimeData(indexes);
    if (indexes.size() == 1) {
        auto instanceId = data(indexes[0], InstanceIDRole).toString();
        mimeData->setData("application/x-instanceid", instanceId.toUtf8());
    }
    return mimeData;
}

QStringList InstanceList::getLinkedInstancesById(const QString& id) const
{
    QStringList linkedInstances;
    for (const auto& inst : m_instances) {
        if (inst->isLinkedToInstanceId(id)) {
            linkedInstances.append(inst->id());
        }
    }
    return linkedInstances;
}

int InstanceList::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return count();
}

QModelIndex InstanceList::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= count()) {
        return {};
    }
    return createIndex(row, column, m_instances.at(row).get());
}

QVariant InstanceList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    auto* pdata = static_cast<BaseInstance*>(index.internalPointer());
    switch (role) {
        case InstancePointerRole: {
            QVariant v = QVariant::fromValue((void*)pdata);
            return v;
        }
        case InstanceIDRole: {
            return pdata->id();
        }
        case Qt::EditRole:
        case Qt::DisplayRole: {
            return pdata->name();
        }
        case Qt::AccessibleTextRole: {
            return tr("%1 Instance").arg(pdata->name());
        }
        case Qt::ToolTipRole: {
            return pdata->instanceRoot();
        }
        case Qt::DecorationRole: {
            return pdata->iconKey();
        }
        // HACK: see InstanceView.h in gui!
        case GroupRole: {
            return getInstanceGroup(pdata->id());
        }
        default:
            break;
    }
    return QVariant();
}

bool InstanceList::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (role != Qt::EditRole) {
        return false;
    }
    auto* pdata = static_cast<BaseInstance*>(index.internalPointer());
    auto newName = value.toString();
    if (pdata->name() == newName) {
        return true;
    }
    pdata->setName(newName);
    return true;
}

Qt::ItemFlags InstanceList::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f;
    if (index.isValid()) {
        f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
    return f;
}

GroupId InstanceList::getInstanceGroup(const InstanceId& id) const
{
    auto* inst = getInstanceById(id);
    if (!inst) {
        return {};
    }
    auto iter = m_instanceGroupIndex.find(inst->id());
    if (iter != m_instanceGroupIndex.end()) {
        return *iter;
    }
    return {};
}

void InstanceList::setInstanceGroup(const InstanceId& id, GroupId name)
{
    if (name.isEmpty() && !name.isNull()) {
        name = QString();
    }

    auto* inst = getInstanceById(id);
    if (!inst) {
        qDebug() << "Attempt to set a null instance's group";
        return;
    }

    bool changed = false;
    auto iter = m_instanceGroupIndex.find(inst->id());
    if (iter != m_instanceGroupIndex.end()) {
        if (*iter != name) {
            decreaseGroupCount(*iter);
            *iter = name;
            changed = true;
        }
    } else {
        changed = true;
        m_instanceGroupIndex[id] = name;
    }

    if (changed) {
        increaseGroupCount(name);
        auto idx = getInstIndex(inst);
        emit dataChanged(index(idx), index(idx), { GroupRole });
        saveGroupList();
    }
}

QStringList InstanceList::getGroups()
{
    return m_groupNameCache.keys();
}

void InstanceList::deleteGroup(const GroupId& name)
{
    m_groupNameCache.remove(name);
    m_collapsedGroups.remove(name);

    bool removed = false;
    qDebug() << "Delete group" << name;
    for (auto& instance : m_instances) {
        const QString& instID = instance->id();
        const QString instGroupName = getInstanceGroup(instID);
        if (instGroupName == name) {
            m_instanceGroupIndex.remove(instID);
            qDebug() << "Remove" << instID << "from group" << name;
            removed = true;
            auto idx = getInstIndex(instance.get());
            if (idx >= 0) {
                emit dataChanged(index(idx), index(idx), { GroupRole });
            }
        }
    }
    if (removed) {
        saveGroupList();
    }
}

void InstanceList::renameGroup(const QString& src, const QString& dst)
{
    m_groupNameCache.remove(src);
    if (m_collapsedGroups.remove(src)) {
        m_collapsedGroups.insert(dst);
    }

    bool modified = false;
    qDebug() << "Rename group" << src << "to" << dst;
    for (auto& instance : m_instances) {
        const QString& instID = instance->id();
        const QString instGroupName = getInstanceGroup(instID);
        if (instGroupName == src) {
            m_instanceGroupIndex[instID] = dst;
            increaseGroupCount(dst);
            qDebug() << "Set" << instID << "group to" << dst;
            modified = true;
            auto idx = getInstIndex(instance.get());
            if (idx >= 0) {
                emit dataChanged(index(idx), index(idx), { GroupRole });
            }
        }
    }
    if (modified) {
        saveGroupList();
    }
}

bool InstanceList::isGroupCollapsed(const QString& group)
{
    return m_collapsedGroups.contains(group);
}

bool InstanceList::trashInstance(const InstanceId& id)
{
    auto* inst = getInstanceById(id);
    if (!inst) {
        qWarning() << "Cannot trash instance" << id << ". No such instance is present (deleted externally?).";
        return false;
    }

    QString cachedGroupId = m_instanceGroupIndex[id];

    qDebug() << "Will trash instance" << id;
    QString trashedLoc;

    if (m_instanceGroupIndex.remove(id) != 0) {
        decreaseGroupCount(cachedGroupId);
        saveGroupList();
    }

    if (!FS::trash(inst->instanceRoot(), &trashedLoc)) {
        qWarning() << "Trash of instance" << id << "has not been completely successful...";
        return false;
    }

    qDebug() << "Instance" << id << "has been trashed by the launcher.";
    m_trashHistory.push({ id, inst->instanceRoot(), trashedLoc, cachedGroupId });

    // Also trash all of its shortcuts; we remove the shortcuts if trash fails since it is invalid anyway
    for (const auto& [name, filePath, target] : inst->shortcuts()) {
        if (!FS::trash(filePath, &trashedLoc)) {
            qWarning() << "Trash of shortcut" << name << "at path" << filePath << "for instance" << id
                       << "has not been successful, trying to delete it instead...";
            if (!FS::deletePath(filePath)) {
                qWarning() << "Deletion of shortcut" << name << "at path" << filePath << "for instance" << id
                           << "has not been successful, given up...";
            } else {
                qDebug() << "Shortcut" << name << "at path" << filePath << "for instance" << id << "has been deleted by the launcher.";
            }
            continue;
        }
        qDebug() << "Shortcut" << name << "at path" << filePath << "for instance" << id << "has been trashed by the launcher.";
        m_trashHistory.top().shortcuts.append({ { name, filePath, target }, trashedLoc });
    }

    return true;
}

bool InstanceList::trashedSomething() const
{
    return !m_trashHistory.empty();
}

bool InstanceList::undoTrashInstance()
{
    if (m_trashHistory.empty()) {
        qWarning() << "Nothing to recover from trash.";
        return true;
    }

    auto top = m_trashHistory.pop();

    while (QDir(top.path).exists()) {
        top.id += "1";
        top.path += "1";
    }

    if (!QFile(top.trashPath).rename(top.path)) {
        qWarning() << "Moving" << top.trashPath << "back to" << top.path << "failed!";
        return false;
    }
    qDebug() << "Moving" << top.trashPath << "back to" << top.path;

    bool ok = true;
    for (const auto& [data, trashPath] : top.shortcuts) {
        if (QDir(data.filePath).exists()) {
            // Don't try to append 1 here as the shortcut may have suffixes like .app, just warn and skip it
            qWarning() << "Shortcut" << trashPath << "original directory" << data.filePath << "already exists!";
            ok = false;
            continue;
        }
        if (!QFile(trashPath).rename(data.filePath)) {
            qWarning() << "Moving shortcut from" << trashPath << "back to" << data.filePath << "failed!";
            ok = false;
            continue;
        }
        qDebug() << "Moving shortcut from" << trashPath << "back to" << data.filePath;
    }

    m_instanceGroupIndex[top.id] = top.groupName;
    increaseGroupCount(top.groupName);

    saveGroupList();
    emit instancesChanged();
    return ok;
}

void InstanceList::deleteInstance(const InstanceId& id)
{
    auto* inst = getInstanceById(id);
    if (!inst) {
        qWarning() << "Cannot delete instance" << id << ". No such instance is present (deleted externally?).";
        return;
    }

    QString cachedGroupId = m_instanceGroupIndex[id];

    if (m_instanceGroupIndex.remove(id) != 0) {
        decreaseGroupCount(cachedGroupId);
        saveGroupList();
    }

    qDebug() << "Will delete instance" << id;
    if (!FS::deletePath(inst->instanceRoot())) {
        qWarning() << "Deletion of instance" << id << "has not been completely successful...";
        return;
    }

    qDebug() << "Instance" << id << "has been deleted by the launcher.";

    for (const auto& [name, filePath, target] : inst->shortcuts()) {
        if (!FS::deletePath(filePath)) {
            qWarning() << "Deletion of shortcut" << name << "at path" << filePath << "for instance" << id << "has not been successful...";
            continue;
        }
        qDebug() << "Shortcut" << name << "at path" << filePath << "for instance" << id << "has been deleted by the launcher.";
    }
}

namespace {
QMap<InstanceId, InstanceLocator> getIdMapping(const std::vector<std::unique_ptr<BaseInstance>>& list)
{
    QMap<InstanceId, InstanceLocator> out;
    int i = 0;
    for (const auto& item : list) {
        auto id = item->id();
        if (out.contains(id)) {
            qWarning() << "Duplicate ID" << id << "in instance list";
        }
        out[id] = std::make_pair(item.get(), i);
        i++;
    }
    return out;
}
}  // namespace

QList<InstanceId> InstanceList::discoverInstances()
{
    QList<InstanceId> out;
    m_instanceRootDirMap.clear();
    for (const auto& rootDir : m_instDirs) {
        qInfo() << "Discovering instances in" << rootDir;
        QDirIterator iter(rootDir, QDir::Dirs | QDir::NoDot | QDir::NoDotDot | QDir::Readable | QDir::Hidden, QDirIterator::FollowSymlinks);
        while (iter.hasNext()) {
            QString subDir = iter.next();
            QFileInfo dirInfo(subDir);
            if (!QFileInfo(FS::PathCombine(subDir, "instance.cfg")).exists())
                continue;
            // if it is a symlink, ignore it if it goes to ANY configured instance
            if (dirInfo.isSymLink()) {
                QFileInfo targetInfo(dirInfo.symLinkTarget());
                QString targetCanonical = targetInfo.canonicalFilePath();
                bool pointsIntoAnyRoot = std::ranges::any_of(m_instDirs, [&targetCanonical](const QString& otherRoot) {
                    return targetCanonical.startsWith(QFileInfo(otherRoot).canonicalFilePath());
                });
                if (pointsIntoAnyRoot) {
                    qDebug() << "Ignoring symlink" << subDir << "that leads into a configured instance root";
                    continue;
                }
            }
            auto id = dirInfo.fileName();
            if (m_instanceRootDirMap.contains(id)) {
                qWarning() << "Duplicate instance ID" << id << "found in" << rootDir << "- already claimed by"
                           << m_instanceRootDirMap.value(id) << ". Skipping.";
                continue;
            }
            m_instanceRootDirMap[id] = rootDir;
            out.append(id);
            qInfo() << "Found instance ID" << id << "in" << rootDir;
        }
    }
    m_instanceSet = QSet<QString>(out.begin(), out.end());
    m_instancesProbed = true;
    return out;
}

QString InstanceList::rootDirOf(const InstanceId& id) const
{
    return m_instanceRootDirMap.value(id, primaryDir());
}

InstanceList::InstListError InstanceList::loadList()
{
    auto existingIds = getIdMapping(m_instances);

    std::vector<std::unique_ptr<BaseInstance>> newList;

    for (auto& id : discoverInstances()) {
        if (existingIds.contains(id)) {
            existingIds.remove(id);
            qInfo() << "Should keep and soft-reload" << id;
        } else {
            std::unique_ptr<BaseInstance> instPtr = loadInstance(id);
            if (instPtr) {
                newList.push_back(std::move(instPtr));
            }
        }
    }

    // TODO: looks like a general algorithm with a few specifics inserted. Do something about it.
    if (!existingIds.isEmpty()) {
        // get the list of removed instances and sort it by their original index, from last to first
        auto deadList = existingIds.values();
        auto orderSortPredicate = [](const InstanceLocator& a, const InstanceLocator& b) -> bool { return a.second > b.second; };
        std::ranges::sort(deadList, orderSortPredicate);
        // remove the contiguous ranges of rows
        int frontBookmark = -1;
        int backBookmark = -1;
        int currentItem = -1;
        auto removeNow = [this, &frontBookmark, &backBookmark, &currentItem]() {
            beginRemoveRows(QModelIndex(), frontBookmark, backBookmark);
            m_instances.erase(m_instances.begin() + frontBookmark, m_instances.begin() + backBookmark + 1);
            endRemoveRows();
            frontBookmark = -1;
            backBookmark = currentItem;
        };
        for (auto& removedItem : deadList) {
            auto* instPtr = removedItem.first;
            instPtr->invalidate();
            currentItem = removedItem.second;
            if (backBookmark == -1) {
                // no bookmark yet
                backBookmark = currentItem;
            } else if (currentItem == frontBookmark - 1) {
                // part of contiguous sequence, continue
            } else {
                // seam between previous and current item
                removeNow();
            }
            frontBookmark = currentItem;
        }
        if (backBookmark != -1) {
            removeNow();
        }
    }
    if (!newList.empty()) {
        add(newList);
    }
    m_dirty = false;
    updateTotalPlayTime();
    migrateTotalPlayTime();
    return NoError;
}

void InstanceList::updateTotalPlayTime()
{
    m_totalPlayTime = 0;
    for (const auto& itr : m_instances) {
        if (itr->countTimePlayed()) {
            m_totalPlayTime += itr->totalTimePlayed();
        }
    }
}

void InstanceList::migrateTotalPlayTime()
{
    if (m_globalSettings->get("TotalPlayTimeMigrated").toBool()) {
        return;
    }

    qint64 existingTotal = 0;
    for (const auto& itr : m_instances) {
        existingTotal += itr->totalTimePlayed();
    }

    qint64 current = m_globalSettings->get("TotalPlayTime").toLongLong();
    m_globalSettings->set("TotalPlayTime", current + existingTotal);
    m_globalSettings->set("TotalPlayTimeMigrated", true);

    qDebug() << "Migrated" << existingTotal << "seconds of existing instance playtime into global TotalPlayTime.";
}

void InstanceList::saveNow()
{
    for (auto& item : m_instances) {
        item->saveNow();
    }
}

void InstanceList::add(std::vector<std::unique_ptr<BaseInstance>>& t)
{
    beginInsertRows(QModelIndex(), count(), static_cast<int>(count() + t.size() - 1));
    for (auto& ptr : t) {
        m_instances.push_back(std::move(ptr));
        connect(m_instances.back().get(), &BaseInstance::propertiesChanged, this, &InstanceList::propertiesChanged);
    }
    endInsertRows();
}

void InstanceList::resumeWatch()
{
    if (m_watchLevel > 0) {
        qWarning() << "Bad suspend level resume in instance list";
        return;
    }
    m_watchLevel++;
    if (m_watchLevel > 0 && m_dirty) {
        loadList();
    }
}

void InstanceList::suspendWatch()
{
    m_watchLevel--;
}

void InstanceList::providerUpdated()
{
    m_dirty = true;
    if (m_watchLevel == 1) {
        loadList();
    }
}

BaseInstance* InstanceList::getInstanceById(const QString& instId) const
{
    if (instId.isEmpty()) {
        return nullptr;
    }
    for (const auto& inst : m_instances) {
        if (inst->id() == instId || inst->uuid() == instId) {
            return inst.get();
        }
    }
    return nullptr;
}

BaseInstance* InstanceList::getInstanceByManagedName(const QString& managedName) const
{
    if (managedName.isEmpty()) {
        return {};
    }

    for (const auto& instance : m_instances) {
        if (instance->getManagedPackName() == managedName) {
            return instance.get();
        }
    }

    return {};
}

QModelIndex InstanceList::getInstanceIndexById(const QString& id) const
{
    return index(getInstIndex(getInstanceById(id)));
}

int InstanceList::getInstIndex(BaseInstance* inst) const
{
    int count = this->count();
    for (int i = 0; i < count; i++) {
        if (inst == m_instances.at(i).get()) {
            return i;
        }
    }
    return -1;
}

void InstanceList::propertiesChanged(BaseInstance* inst)
{
    int i = getInstIndex(inst);
    if (i != -1) {
        emit dataChanged(index(i), index(i));
        updateTotalPlayTime();
    }
}

std::unique_ptr<BaseInstance> InstanceList::loadInstance(const InstanceId& id)
{
    if (!m_groupsLoaded) {
        loadGroupList();
    }

    auto instanceRoot = FS::PathCombine(rootDirOf(id), id);
    auto instanceSettings = std::make_unique<INISettingsObject>(FS::PathCombine(instanceRoot, "instance.cfg"));
    std::unique_ptr<BaseInstance> inst;

    instanceSettings->registerSetting("InstanceType", "");

    const QString instType = instanceSettings->get("InstanceType").toString();

    // NOTE: Some launcher versions didn't save the InstanceType properly. We will just bank on the probability that this is probably a
    // OneSix instance
    if (instType == "OneSix" || instType.isEmpty()) {
        inst.reset(new MinecraftInstance(m_globalSettings, std::move(instanceSettings), instanceRoot));
    } else {
        inst.reset(new NullInstance(m_globalSettings, std::move(instanceSettings), instanceRoot));
    }
    qDebug() << "Loaded instance" << inst->name() << "from" << inst->instanceRoot();

    auto shortcut = inst->shortcuts();
    if (!shortcut.isEmpty()) {
        qDebug() << "Loaded" << shortcut.size() << "shortcut(s) for instance" << inst->name();
    }

    return inst;
}

void InstanceList::increaseGroupCount(const QString& group)
{
    if (group.isEmpty()) {
        return;
    }

    ++m_groupNameCache[group];
}

void InstanceList::decreaseGroupCount(const QString& group)
{
    if (group.isEmpty()) {
        return;
    }

    if (--m_groupNameCache[group] < 1) {
        m_groupNameCache.remove(group);
        m_collapsedGroups.remove(group);
    }
}

void InstanceList::saveGroupList()
{
    qDebug() << "Will save group list now.";
    if (!m_instancesProbed) {
        qDebug() << "Group saving prevented because we don't know the full list of instances yet.";
        return;
    }

    QString groupFileName = QDir::current().filePath("instgroups.json");

    QMap<QString, QSet<QString>> reverseGroupMap;
    for (auto iter = m_instanceGroupIndex.begin(); iter != m_instanceGroupIndex.end(); iter++) {
        const QString& id = iter.key();
        const QString& group = iter.value();
        if (group.isEmpty()) {
            continue;
        }
        reverseGroupMap[group].insert(id);
    }

    QJsonObject toplevel;
    toplevel.insert("formatVersion", QJsonValue(QString("1")));
    QJsonObject groupsArr;
    for (auto iter = reverseGroupMap.begin(); iter != reverseGroupMap.end(); iter++) {
        auto list = iter.value();
        const auto& name = iter.key();
        QJsonObject groupObj;
        QJsonArray instanceArr;
        groupObj.insert("hidden", QJsonValue(m_collapsedGroups.contains(name)));
        for (const auto& item : list) {
            instanceArr.append(QJsonValue(item));
        }
        groupObj.insert("instances", instanceArr);
        groupsArr.insert(name, groupObj);
    }
    toplevel.insert("groups", groupsArr);
    // empty string represents ungrouped "group"
    if (m_collapsedGroups.contains("")) {
        QJsonObject ungrouped;
        ungrouped.insert("hidden", QJsonValue(true));
        toplevel.insert("ungrouped", ungrouped);
    }
    QJsonDocument doc(toplevel);
    try {
        FS::write(groupFileName, doc.toJson());
        qDebug() << "Group list saved.";
    } catch (const FS::FileSystemException& e) {
        qCritical() << "Failed to write instance group file :" << e.cause();
    }
}

void InstanceList::loadGroupList()
{
    qDebug() << "Will load group list now.";

    QString groupFileName = QDir::current().filePath("instgroups.json");

    m_instanceGroupIndex.clear();
    m_groupNameCache.clear();
    m_collapsedGroups.clear();

    // if there's no group file, fail
    if (!QFileInfo::exists(groupFileName)) {
        return;
    }

    QByteArray jsonData;
    try {
        jsonData = FS::read(groupFileName);
    } catch (const FS::FileSystemException& e) {
        qCritical() << "Failed to read instance group file :" << e.cause();
        return;
    }

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);

    // if the json was bad, fail
    if (error.error != QJsonParseError::NoError) {
        qCritical() << QString("Failed to parse instance group file: %1 at offset %2")
                           .arg(error.errorString(), QString::number(error.offset))
                           .toUtf8();
        return;
    }

    // if the root of the json wasn't an object, fail
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid group file. Root entry should be an object.";
        return;
    }

    QJsonObject rootObj = jsonDoc.object();

    // Make sure the format version matches, otherwise fail.
    if (rootObj.value("formatVersion").toVariant().toInt() != g_GROUP_FILE_FORMAT_VERSION) {
        return;
    }

    // Get the groups. if it's not an object, fail
    if (!rootObj.value("groups").isObject()) {
        qWarning() << "Invalid group list JSON: 'groups' should be an object.";
        return;
    }

    QJsonObject groupMapping = rootObj.value("groups").toObject();
    for (QJsonObject::iterator iter = groupMapping.begin(); iter != groupMapping.end(); iter++) {
        QString groupName = iter.key();
        if (groupName.isEmpty()) {
            qWarning() << "Redundant empty group found";
            continue;
        }
        if (!iter.value().isObject()) {
            qWarning() << QString("Group '%1' in the group list should be an object").arg(groupName).toUtf8();
            continue;
        }

        QJsonObject groupObj = iter.value().toObject();
        if (!groupObj.value("instances").isArray()) {
            qWarning() << QString("Group '%1' in the group list is invalid. It should contain an array called 'instances'.")
                              .arg(groupName)
                              .toUtf8();
            continue;
        }

        auto hidden = groupObj.value("hidden").toBool(false);
        if (hidden) {
            m_collapsedGroups.insert(groupName);
        }

        QJsonArray instancesArray = groupObj.value("instances").toArray();
        for (auto value : instancesArray) {
            m_instanceGroupIndex[value.toString()] = groupName;
            increaseGroupCount(groupName);
        }
    }

    bool ungroupedHidden = false;
    if (rootObj.value("ungrouped").isObject()) {
        ungroupedHidden = rootObj.value("ungrouped").toObject().value("hidden").toBool(false);
    }
    if (ungroupedHidden) {
        // empty string represents ungrouped "group"
        m_collapsedGroups.insert("");
    }
    m_groupsLoaded = true;
    qDebug() << "Group list loaded.";
}

void InstanceList::instanceDirContentsChanged(const QString& path)
{
    Q_UNUSED(path);
    emit instancesChanged();
}

void InstanceList::on_InstFolderChanged([[maybe_unused]] const Setting& setting, [[maybe_unused]] const QVariant& value)
{
    QString instDir = m_globalSettings->get("InstanceDir").toString();
    QStringList additionalDirs = m_globalSettings->get("AdditionalInstanceDirs").toStringList();

    QStringList newDirs;
    QStringList candidates;
    candidates << instDir << additionalDirs;
    for (const auto& dir : candidates) {
        if (dir.isEmpty())
            continue;
        QDir::current().mkpath(dir);
        QString canonical = QDir(dir).canonicalPath();
        if (!canonical.isEmpty() && !newDirs.contains(canonical))
            newDirs << canonical;
    }

    if (newDirs != m_instDirs) {
        if (m_groupsLoaded) {
            saveGroupList();
        }
        for (const auto& dir : m_instDirs)
            m_watcher->removePath(dir);
        m_instDirs = newDirs;
        for (const auto& dir : m_instDirs)
            m_watcher->addPath(dir);
        m_groupsLoaded = false;
        beginRemoveRows(QModelIndex(), 0, count());
        m_instances.erase(m_instances.begin(), m_instances.end());
        endRemoveRows();
        emit instancesChanged();
    }
}

void InstanceList::on_GroupStateChanged(const QString& group, bool collapsed)
{
    qDebug() << "Group" << group << (collapsed ? "collapsed" : "expanded");
    if (collapsed) {
        m_collapsedGroups.insert(group);
    } else {
        m_collapsedGroups.remove(group);
    }
    saveGroupList();
}

namespace {

class InstanceStaging : public Task {
    Q_OBJECT
    const unsigned minBackoff = 1;
    const unsigned maxBackoff = 16;

   public:
    InstanceStaging(InstanceList* parent, InstanceTask* child, SettingsObject* settings)
        : m_parent(parent), m_backoff(minBackoff, maxBackoff)
    {
        m_stagingPath = parent->getStagedInstancePath();

        m_child.reset(child);

        m_child->setStagingPath(m_stagingPath);
        m_child->setParentSettings(settings);

        connect(child, &Task::succeeded, this, &InstanceStaging::childSucceeded);
        connect(child, &Task::failed, this, &InstanceStaging::childFailed);
        connect(child, &Task::aborted, this, &InstanceStaging::childAborted);
        connect(child, &Task::abortStatusChanged, this, &InstanceStaging::setAbortable);
        connect(child, &Task::abortButtonTextChanged, this, &InstanceStaging::setAbortButtonText);
        connect(child, &Task::status, this, &InstanceStaging::setStatus);
        connect(child, &Task::details, this, &InstanceStaging::setDetails);
        connect(child, &Task::progress, this, &InstanceStaging::setProgress);
        connect(child, &Task::stepProgress, this, &InstanceStaging::propagateStepProgress);
        connect(&m_backoffTimer, &QTimer::timeout, this, &InstanceStaging::childSucceeded);
    }

    ~InstanceStaging() override = default;

    // FIXME/TODO: add ability to abort during instance commit retries
    bool abort() override
    {
        if (!canAbort()) {
            return false;
        }

        return m_child->abort();
    }
    bool canAbort() const override { return (m_child && m_child->canAbort()); }

   protected:
    void executeTask() override
    {
        if (m_stagingPath.isNull()) {
            emitFailed(tr("Could not create staging folder"));
            return;
        }

        m_child->start();
    }
    QStringList warnings() const override { return m_child->warnings(); }

   private slots:
    void childSucceeded()
    {
        const unsigned sleepTime = m_backoff();
        if (m_parent->commitStagedInstance(m_stagingPath, *m_child, m_child->group())) {
            m_backoffTimer.stop();
            emitSucceeded();
            return;
        }
        // we actually failed, retry?
        if (sleepTime == maxBackoff) {
            m_backoffTimer.stop();
            emitFailed(tr("Failed to commit instance, even after multiple retries. It is being blocked by something."));
            return;
        }
        qDebug() << "Failed to commit instance" << m_child->name() << "Initiating backoff:" << sleepTime;
        m_backoffTimer.start(sleepTime * 500);
    }
    void childFailed(const QString& reason)
    {
        m_backoffTimer.stop();
        FS::deletePath(m_stagingPath);
        emitFailed(reason);
    }

    void childAborted()
    {
        m_backoffTimer.stop();
        FS::deletePath(m_stagingPath);
        emitAborted();
    }

   private:
    InstanceList* m_parent;
    /*
     * WHY: the whole reason why this uses an exponential backoff retry scheme is antivirus on Windows.
     * Basically, it starts messing things up while the launcher is extracting/creating instances
     * and causes that horrible failure that is NTFS to lock files in place because they are open.
     */
    ExponentialSeries m_backoff;
    QString m_stagingPath;
    std::unique_ptr<InstanceTask> m_child;
    QTimer m_backoffTimer;
};
}  // namespace

Task* InstanceList::wrapInstanceTask(InstanceTask* task)
{
    return new InstanceStaging(this, task, m_globalSettings);
}

QString InstanceList::getStagedInstancePath()
{
    const QString tempRoot = FS::PathCombine(primaryDir(), ".tmp");

    QString result;
    int tries = 0;

    do {
        if (++tries > 256) {
            return {};
        }

        const QString key = QUuid::createUuid().toString(QUuid::Id128).left(6);
        result = FS::PathCombine(tempRoot, key);
    } while (QFileInfo::exists(result));

    if (!QDir::current().mkpath(result)) {
        return {};
    }
#ifdef Q_OS_WIN32
    SetFileAttributesA(tempRoot.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
    return result;
}

bool InstanceList::commitStagedInstance(const QString& path, const InstanceTask& instanceTask, QString groupName)
{
    if (groupName.isEmpty() && !groupName.isNull()) {
        groupName = QString();
    }

    QString instID;

    auto shouldOverride = instanceTask.shouldOverride();

    if (shouldOverride) {
        instID = instanceTask.originalInstanceID();
    } else {
        instID = FS::DirNameFromString(instanceTask.modifiedName(), primaryDir());
    }

    Q_ASSERT(!instID.isEmpty());

    {
        WatchLock lock(m_watcher, primaryDir());
        QString destination = FS::PathCombine(primaryDir(), instID);

        if (shouldOverride) {
            if (!FS::overrideFolder(destination, path)) {
                qWarning() << "Failed to override" << path << "to" << destination;
                return false;
            }
        } else {
            if (!FS::move(path, destination)) {
                qWarning() << "Failed to move" << path << "to" << destination;
                return false;
            }

            m_instanceGroupIndex[instID] = groupName;
            increaseGroupCount(groupName);
            m_instanceRootDirMap[instID] = primaryDir();
        }

        m_instanceSet.insert(instID);

        emit instancesChanged();
        emit instanceSelectRequest(instID);
    }

    saveGroupList();
    return true;
}

int64_t InstanceList::getTotalPlayTime()
{
    updateTotalPlayTime();
    return m_totalPlayTime;
}

#include "InstanceList.moc"
