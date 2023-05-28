// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>
#include <QSet>
#include <QStack>
#include <QPair>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QXmlStreamReader>

#include "BaseInstance.h"
#include "ExponentialSeries.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "InstanceTask.h"
#include "NullInstance.h"
#include "WatchLock.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/INISettingsObject.h"

#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(SettingsObjectPtr settings, const QString& instDir, QObject* parent)
    : QAbstractListModel(parent), m_globalSettings(settings)
{
    resumeWatch();
    // Create aand normalize path
    if (!QDir::current().exists(instDir)) {
        QDir::current().mkpath(instDir);
    }

    connect(this, &InstanceList::instancesChanged, this, &InstanceList::providerUpdated);

    // NOTE: canonicalPath requires the path to exist. Do not move this above the creation block!
    m_instDir = QDir(instDir).canonicalPath();
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &InstanceList::instanceDirContentsChanged);
    m_watcher->addPath(m_instDir);
}

InstanceList::~InstanceList() {}

Qt::DropActions InstanceList::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions InstanceList::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool InstanceList::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    if (data && data->hasFormat("application/x-instanceid")) {
        return true;
    }
    return false;
}

bool InstanceList::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (data && data->hasFormat("application/x-instanceid")) {
        return true;
    }
    return false;
}

QStringList InstanceList::mimeTypes() const
{
    auto types = QAbstractListModel::mimeTypes();
    types.push_back("application/x-instanceid");
    return types;
}

QMimeData* InstanceList::mimeData(const QModelIndexList& indexes) const
{
    auto mimeData = QAbstractListModel::mimeData(indexes);
    if (indexes.size() == 1) {
        auto instanceId = data(indexes[0], InstanceIDRole).toString();
        mimeData->setData("application/x-instanceid", instanceId.toUtf8());
    }
    return mimeData;
}

QStringList InstanceList::getLinkedInstancesById(const QString &id) const
{
    QStringList linkedInstances;
    for (auto inst : m_instances) {
        if (inst->isLinkedToInstanceId(id))
            linkedInstances.append(inst->id());
    }
    return linkedInstances;
}

int InstanceList::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_instances.count();
}

QModelIndex InstanceList::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= m_instances.size())
        return QModelIndex();
    return createIndex(row, column, (void*)m_instances.at(row).get());
}

QVariant InstanceList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    BaseInstance *pdata = static_cast<BaseInstance *>(index.internalPointer());
    switch (role)
    {
    case InstancePointerRole:
    {
        QVariant v = QVariant::fromValue((void *)pdata);
        return v;
    }
    case InstanceIDRole:
    {
        return pdata->id();
    }
    case Qt::EditRole:
    case Qt::DisplayRole:
    {
        return pdata->name();
    }
    case Qt::AccessibleTextRole:
    {
        return tr("%1 Instance").arg(pdata->name());
    }
    case Qt::ToolTipRole:
    {
        return pdata->instanceRoot();
    }
    case Qt::DecorationRole:
    {
        return pdata->iconKey();
    }
    // HACK: see InstanceView.h in gui!
    case GroupRole:
    {
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
    BaseInstance* pdata = static_cast<BaseInstance*>(index.internalPointer());
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
    auto inst = getInstanceById(id);
    if (!inst) {
        return GroupId();
    }
    auto iter = m_instanceGroupIndex.find(inst->id());
    if (iter != m_instanceGroupIndex.end()) {
        return *iter;
    }
    return GroupId();
}

void InstanceList::setInstanceGroup(const InstanceId& id, const GroupId& name)
{
    auto inst = getInstanceById(id);
    if (!inst) {
        qDebug() << "Attempt to set a null instance's group";
        return;
    }

    bool changed = false;
    auto iter = m_instanceGroupIndex.find(inst->id());
    if (iter != m_instanceGroupIndex.end()) {
        if (*iter != name) {
            *iter = name;
            changed = true;
        }
    } else {
        changed = true;
        m_instanceGroupIndex[id] = name;
    }

    if (changed) {
        m_groupNameCache.insert(name);
        auto idx = getInstIndex(inst.get());
        emit dataChanged(index(idx), index(idx), { GroupRole });
        saveGroupList();
    }
}

QStringList InstanceList::getGroups()
{
    return m_groupNameCache.values();
}

void InstanceList::deleteGroup(const QString& name)
{
    bool removed = false;
    qDebug() << "Delete group" << name;
    for (auto& instance : m_instances) {
        const auto& instID = instance->id();
        auto instGroupName = getInstanceGroup(instID);
        if (instGroupName == name) {
            m_instanceGroupIndex.remove(instID);
            qDebug() << "Remove" << instID << "from group" << name;
            removed = true;
            auto idx = getInstIndex(instance.get());
            if (idx > 0) {
                emit dataChanged(index(idx), index(idx), { GroupRole });
            }
        }
    }
    if (removed) {
        saveGroupList();
    }
}

bool InstanceList::isGroupCollapsed(const QString& group)
{
    return m_collapsedGroups.contains(group);
}

bool InstanceList::trashInstance(const InstanceId& id)
{
    auto inst = getInstanceById(id);
    if (!inst) {
        qDebug() << "Cannot trash instance" << id << ". No such instance is present (deleted externally?).";
        return false;
    }

    auto cachedGroupId = m_instanceGroupIndex[id];

    qDebug() << "Will trash instance" << id;
    QString trashedLoc;

    if (m_instanceGroupIndex.remove(id)) {
        saveGroupList();
    }

    if (!FS::trash(inst->instanceRoot(), &trashedLoc)) {
        qDebug() << "Trash of instance" << id << "has not been completely successfully...";
        return false;
    }

    qDebug() << "Instance" << id << "has been trashed by the launcher.";
    m_trashHistory.push({id, inst->instanceRoot(), trashedLoc, cachedGroupId});
    
    return true;
}

bool InstanceList::trashedSomething() {
    return !m_trashHistory.empty();
}

void InstanceList::undoTrashInstance() {
    if (m_trashHistory.empty()) {
        qWarning() << "Nothing to recover from trash.";
        return;
    }

    auto top = m_trashHistory.pop();

    while (QDir(top.polyPath).exists()) {
        top.id += "1";
        top.polyPath += "1";
    }

    qDebug() << "Moving" << top.trashPath << "back to" << top.polyPath;
    QFile(top.trashPath).rename(top.polyPath);

    m_instanceGroupIndex[top.id] = top.groupName;
    m_groupNameCache.insert(top.groupName);

    saveGroupList();
    emit instancesChanged();
}

void InstanceList::deleteInstance(const InstanceId& id)
{
    auto inst = getInstanceById(id);
    if (!inst) {
        qDebug() << "Cannot delete instance" << id << ". No such instance is present (deleted externally?).";
        return;
    }

    if (m_instanceGroupIndex.remove(id)) {
        saveGroupList();
    }

    qDebug() << "Will delete instance" << id;
    if (!FS::deletePath(inst->instanceRoot())) {
        qWarning() << "Deletion of instance" << id << "has not been completely successful ...";
        return;
    }

    qDebug() << "Instance" << id << "has been deleted by the launcher.";
}

static QMap<InstanceId, InstanceLocator> getIdMapping(const QList<InstancePtr>& list)
{
    QMap<InstanceId, InstanceLocator> out;
    int i = 0;
    for (auto& item : list) {
        auto id = item->id();
        if (out.contains(id)) {
            qWarning() << "Duplicate ID" << id << "in instance list";
        }
        out[id] = std::make_pair(item, i);
        i++;
    }
    return out;
}

QList<InstanceId> InstanceList::discoverInstances()
{
    qDebug() << "Discovering instances in" << m_instDir;
    QList<InstanceId> out;
    QDirIterator iter(m_instDir, QDir::Dirs | QDir::NoDot | QDir::NoDotDot | QDir::Readable | QDir::Hidden, QDirIterator::FollowSymlinks);
    while (iter.hasNext()) {
        QString subDir = iter.next();
        QFileInfo dirInfo(subDir);
        if (!QFileInfo(FS::PathCombine(subDir, "instance.cfg")).exists())
            continue;
        // if it is a symlink, ignore it if it goes to the instance folder
        if (dirInfo.isSymLink()) {
            QFileInfo targetInfo(dirInfo.symLinkTarget());
            QFileInfo instDirInfo(m_instDir);
            if (targetInfo.canonicalPath() == instDirInfo.canonicalFilePath()) {
                qDebug() << "Ignoring symlink" << subDir << "that leads into the instances folder";
                continue;
            }
        }
        auto id = dirInfo.fileName();
        out.append(id);
        qDebug() << "Found instance ID" << id;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    instanceSet = QSet<QString>(out.begin(), out.end());
#else
    instanceSet = out.toSet();
#endif
    m_instancesProbed = true;
    return out;
}

InstanceList::InstListError InstanceList::loadList()
{
    auto existingIds = getIdMapping(m_instances);

    QList<InstancePtr> newList;

    for (auto& id : discoverInstances()) {
        if (existingIds.contains(id)) {
            auto instPair = existingIds[id];
            existingIds.remove(id);
            qDebug() << "Should keep and soft-reload" << id;
        } else {
            InstancePtr instPtr = loadInstance(id);
            if (instPtr) {
                newList.append(instPtr);
            }
        }
    }

    // TODO: looks like a general algorithm with a few specifics inserted. Do something about it.
    if (!existingIds.isEmpty()) {
        // get the list of removed instances and sort it by their original index, from last to first
        auto deadList = existingIds.values();
        auto orderSortPredicate = [](const InstanceLocator& a, const InstanceLocator& b) -> bool { return a.second > b.second; };
        std::sort(deadList.begin(), deadList.end(), orderSortPredicate);
        // remove the contiguous ranges of rows
        int front_bookmark = -1;
        int back_bookmark = -1;
        int currentItem = -1;
        auto removeNow = [&]() {
            beginRemoveRows(QModelIndex(), front_bookmark, back_bookmark);
            m_instances.erase(m_instances.begin() + front_bookmark, m_instances.begin() + back_bookmark + 1);
            endRemoveRows();
            front_bookmark = -1;
            back_bookmark = currentItem;
        };
        for (auto& removedItem : deadList) {
            auto instPtr = removedItem.first;
            instPtr->invalidate();
            currentItem = removedItem.second;
            if (back_bookmark == -1) {
                // no bookmark yet
                back_bookmark = currentItem;
            } else if (currentItem == front_bookmark - 1) {
                // part of contiguous sequence, continue
            } else {
                // seam between previous and current item
                removeNow();
            }
            front_bookmark = currentItem;
        }
        if (back_bookmark != -1) {
            removeNow();
        }
    }
    if (newList.size()) {
        add(newList);
    }
    m_dirty = false;
    updateTotalPlayTime();
    return NoError;
}

void InstanceList::updateTotalPlayTime()
{
    totalPlayTime = 0;
    for (auto const& itr : m_instances) {
        totalPlayTime += itr.get()->totalTimePlayed();
    }
}

void InstanceList::saveNow()
{
    for (auto& item : m_instances) {
        item->saveNow();
    }
}

void InstanceList::add(const QList<InstancePtr>& t)
{
    beginInsertRows(QModelIndex(), m_instances.count(), m_instances.count() + t.size() - 1);
    m_instances.append(t);
    for (auto& ptr : t) {
        connect(ptr.get(), &BaseInstance::propertiesChanged, this, &InstanceList::propertiesChanged);
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

InstancePtr InstanceList::getInstanceById(QString instId) const
{
    if (instId.isEmpty())
        return InstancePtr();
    for (auto& inst : m_instances) {
        if (inst->id() == instId) {
            return inst;
        }
    }
    return InstancePtr();
}

InstancePtr InstanceList::getInstanceByManagedName(const QString& managed_name) const
{
    if (managed_name.isEmpty())
        return {};

    for (auto instance : m_instances) {
        if (instance->getManagedPackName() == managed_name)
            return instance;
    }

    return {};
}

QModelIndex InstanceList::getInstanceIndexById(const QString &id) const
{
    return index(getInstIndex(getInstanceById(id).get()));
}

int InstanceList::getInstIndex(BaseInstance* inst) const
{
    int count = m_instances.count();
    for (int i = 0; i < count; i++) {
        if (inst == m_instances[i].get()) {
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

InstancePtr InstanceList::loadInstance(const InstanceId& id)
{
    if (!m_groupsLoaded) {
        loadGroupList();
    }

    auto instanceRoot = FS::PathCombine(m_instDir, id);
    auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(instanceRoot, "instance.cfg"));
    InstancePtr inst;

    instanceSettings->registerSetting("InstanceType", "");

    QString inst_type = instanceSettings->get("InstanceType").toString();

    // NOTE: Some PolyMC versions didn't save the InstanceType properly. We will just bank on the probability that this is probably a OneSix instance
    if (inst_type == "OneSix" || inst_type.isEmpty())
    {
        inst.reset(new MinecraftInstance(m_globalSettings, instanceSettings, instanceRoot));
    }
    else
    {
        inst.reset(new NullInstance(m_globalSettings, instanceSettings, instanceRoot));
    }
    qDebug() << "Loaded instance " << inst->name() << " from " << inst->instanceRoot();
    return inst;
}

void InstanceList::saveGroupList()
{
    qDebug() << "Will save group list now.";
    if (!m_instancesProbed) {
        qDebug() << "Group saving prevented because we don't know the full list of instances yet.";
        return;
    }
    WatchLock foo(m_watcher, m_instDir);
    QString groupFileName = m_instDir + "/instgroups.json";
    QMap<QString, QSet<QString>> reverseGroupMap;
    for (auto iter = m_instanceGroupIndex.begin(); iter != m_instanceGroupIndex.end(); iter++) {
        QString id = iter.key();
        QString group = iter.value();
        if (group.isEmpty())
            continue;
        if (!instanceSet.contains(id)) {
            qDebug() << "Skipping saving missing instance" << id << "to groups list.";
            continue;
        }

        if (!reverseGroupMap.count(group)) {
            QSet<QString> set;
            set.insert(id);
            reverseGroupMap[group] = set;
        } else {
            QSet<QString>& set = reverseGroupMap[group];
            set.insert(id);
        }
    }
    QJsonObject toplevel;
    toplevel.insert("formatVersion", QJsonValue(QString("1")));
    QJsonObject groupsArr;
    for (auto iter = reverseGroupMap.begin(); iter != reverseGroupMap.end(); iter++) {
        auto list = iter.value();
        auto name = iter.key();
        QJsonObject groupObj;
        QJsonArray instanceArr;
        groupObj.insert("hidden", QJsonValue(m_collapsedGroups.contains(name)));
        for (auto item : list) {
            instanceArr.append(QJsonValue(item));
        }
        groupObj.insert("instances", instanceArr);
        groupsArr.insert(name, groupObj);
    }
    toplevel.insert("groups", groupsArr);
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

    QString groupFileName = m_instDir + "/instgroups.json";

    // if there's no group file, fail
    if (!QFileInfo(groupFileName).exists())
        return;

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
    if (rootObj.value("formatVersion").toVariant().toInt() != GROUP_FILE_FORMAT_VERSION)
        return;

    // Get the groups. if it's not an object, fail
    if (!rootObj.value("groups").isObject()) {
        qWarning() << "Invalid group list JSON: 'groups' should be an object.";
        return;
    }

    QSet<QString> groupSet;
    m_instanceGroupIndex.clear();

    // Iterate through all the groups.
    QJsonObject groupMapping = rootObj.value("groups").toObject();
    for (QJsonObject::iterator iter = groupMapping.begin(); iter != groupMapping.end(); iter++) {
        QString groupName = iter.key();

        // If not an object, complain and skip to the next one.
        if (!iter.value().isObject()) {
            qWarning() << QString("Group '%1' in the group list should be an object.").arg(groupName).toUtf8();
            continue;
        }

        QJsonObject groupObj = iter.value().toObject();
        if (!groupObj.value("instances").isArray()) {
            qWarning() << QString("Group '%1' in the group list is invalid. It should contain an array called 'instances'.")
                              .arg(groupName)
                              .toUtf8();
            continue;
        }

        // keep a list/set of groups for choosing
        groupSet.insert(groupName);

        auto hidden = groupObj.value("hidden").toBool(false);
        if (hidden) {
            m_collapsedGroups.insert(groupName);
        }

        // Iterate through the list of instances in the group.
        QJsonArray instancesArray = groupObj.value("instances").toArray();

        for (QJsonArray::iterator iter2 = instancesArray.begin(); iter2 != instancesArray.end(); iter2++) {
            m_instanceGroupIndex[(*iter2).toString()] = groupName;
        }
    }
    m_groupsLoaded = true;
    m_groupNameCache.unite(groupSet);
    qDebug() << "Group list loaded.";
}

void InstanceList::instanceDirContentsChanged(const QString& path)
{
    Q_UNUSED(path);
    emit instancesChanged();
}

void InstanceList::on_InstFolderChanged(const Setting& setting, QVariant value)
{
    QString newInstDir = QDir(value.toString()).canonicalPath();
    if (newInstDir != m_instDir) {
        if (m_groupsLoaded) {
            saveGroupList();
        }
        m_instDir = newInstDir;
        m_groupsLoaded = false;
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

class InstanceStaging : public Task {
    Q_OBJECT
    const unsigned minBackoff = 1;
    const unsigned maxBackoff = 16;
   public:
    InstanceStaging(InstanceList* parent, InstanceTask* child, QString stagingPath, InstanceName const& instanceName, QString groupName)
        : m_parent(parent), backoff(minBackoff, maxBackoff), m_stagingPath(std::move(stagingPath)), m_instance_name(std::move(instanceName)), m_groupName(std::move(groupName))
    {
        m_child.reset(child);
        connect(child, &Task::succeeded, this, &InstanceStaging::childSucceded);
        connect(child, &Task::failed, this, &InstanceStaging::childFailed);
        connect(child, &Task::aborted, this, &InstanceStaging::childAborted);
        connect(child, &Task::abortStatusChanged, this, &InstanceStaging::setAbortable);
        connect(child, &Task::status, this, &InstanceStaging::setStatus);
        connect(child, &Task::details, this, &InstanceStaging::setDetails);
        connect(child, &Task::progress, this, &InstanceStaging::setProgress);
        connect(child, &Task::stepProgress, this, &InstanceStaging::propogateStepProgress);
        connect(&m_backoffTimer, &QTimer::timeout, this, &InstanceStaging::childSucceded);
    }

    virtual ~InstanceStaging(){};

    // FIXME/TODO: add ability to abort during instance commit retries
    bool abort() override
    {
        if (!canAbort())
            return false;

        m_child->abort();

        return Task::abort();
    }
    bool canAbort() const override
    {
        return (m_child && m_child->canAbort());
    }

   protected:
    virtual void executeTask() override { m_child->start(); }
    QStringList warnings() const override { return m_child->warnings(); }

   private slots:
    void childSucceded()
    {
        unsigned sleepTime = backoff();
        if (m_parent->commitStagedInstance(m_stagingPath, m_instance_name, m_groupName, *m_child.get()))
        {
            emitSucceeded();
            return;
        }
        // we actually failed, retry?
        if (sleepTime == maxBackoff) {
            emitFailed(tr("Failed to commit instance, even after multiple retries. It is being blocked by something."));
            return;
        }
        qDebug() << "Failed to commit instance" << m_instance_name.name() << "Initiating backoff:" << sleepTime;
        m_backoffTimer.start(sleepTime * 500);
    }
    void childFailed(const QString& reason)
    {
        m_parent->destroyStagingPath(m_stagingPath);
        emitFailed(reason);
    }

    void childAborted()
    {
        emitAborted();
    }

private:
    InstanceList * m_parent;
    /*
     * WHY: the whole reason why this uses an exponential backoff retry scheme is antivirus on Windows.
     * Basically, it starts messing things up while the launcher is extracting/creating instances
     * and causes that horrible failure that is NTFS to lock files in place because they are open.
     */
    ExponentialSeries backoff;
    QString m_stagingPath;
    unique_qobject_ptr<InstanceTask> m_child;
    InstanceName m_instance_name;
    QString m_groupName;
    QTimer m_backoffTimer;
};

Task* InstanceList::wrapInstanceTask(InstanceTask* task)
{
    auto stagingPath = getStagedInstancePath();
    task->setStagingPath(stagingPath);
    task->setParentSettings(m_globalSettings);
    return new InstanceStaging(this, task, stagingPath, *task, task->group());
}

QString InstanceList::getStagedInstancePath()
{
    QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString tempDir = ".LAUNCHER_TEMP/";
    QString relPath = FS::PathCombine(tempDir, key);
    QDir rootPath(m_instDir);
    auto path = FS::PathCombine(m_instDir, relPath);
    if (!rootPath.mkpath(relPath)) {
        return QString();
    }
#ifdef Q_OS_WIN32
    auto tempPath = FS::PathCombine(m_instDir, tempDir);
    SetFileAttributesA(tempPath.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
    return path;
}

bool InstanceList::commitStagedInstance(const QString& path, InstanceName const& instanceName, const QString& groupName, InstanceTask const& commiting)
{
    QDir dir;
    QString instID;
    InstancePtr inst;

    auto should_override = commiting.shouldOverride();

    if (should_override) {
        instID = commiting.originalInstanceID();
    } else {
        instID = FS::DirNameFromString(instanceName.modifiedName(), m_instDir);
    }

    Q_ASSERT(!instID.isEmpty());

    {
        WatchLock lock(m_watcher, m_instDir);
        QString destination = FS::PathCombine(m_instDir, instID);

        if (should_override) {
            if (!FS::overrideFolder(destination, path)) {
                qWarning() << "Failed to override" << path << "to" << destination;
                return false;
            }
        } else {
            if (!dir.rename(path, destination)) {
                qWarning() << "Failed to move" << path << "to" << destination;
                return false;
            }

            m_instanceGroupIndex[instID] = groupName;
            m_groupNameCache.insert(groupName);
        }

        instanceSet.insert(instID);

        emit instancesChanged();
        emit instanceSelectRequest(instID);
    }

    saveGroupList();
    return true;
}

bool InstanceList::destroyStagingPath(const QString& keyPath)
{
    return FS::deletePath(keyPath);
}

int InstanceList::getTotalPlayTime()
{
    updateTotalPlayTime();
    return totalPlayTime;
}

#include "InstanceList.moc"
