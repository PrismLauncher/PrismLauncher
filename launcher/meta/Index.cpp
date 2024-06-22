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

#include "Index.h"

#include "JsonFormat.h"
#include "QObjectPtr.h"
#include "VersionList.h"
#include "meta/BaseEntity.h"
#include "tasks/SequentialTask.h"

namespace Meta {
Index::Index(QObject* parent) : QAbstractListModel(parent) {}
Index::Index(const QVector<VersionList::Ptr>& lists, QObject* parent) : QAbstractListModel(parent), m_lists(lists)
{
    for (int i = 0; i < m_lists.size(); ++i) {
        m_uids.insert(m_lists.at(i)->uid(), m_lists.at(i));
        connectVersionList(i, m_lists.at(i));
    }
}

QVariant Index::data(const QModelIndex& index, int role) const
{
    if (index.parent().isValid() || index.row() < 0 || index.row() >= m_lists.size()) {
        return QVariant();
    }

    VersionList::Ptr list = m_lists.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
            if (index.column() == 0) {
                return list->humanReadable();
            } else {
                break;
            }
        case UidRole:
            return list->uid();
        case NameRole:
            return list->name();
        case ListPtrRole:
            return QVariant::fromValue(list);
    }
    return QVariant();
}

int Index::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_lists.size();
}

int Index::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant Index::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0) {
        return tr("Name");
    } else {
        return QVariant();
    }
}

bool Index::hasUid(const QString& uid) const
{
    return m_uids.contains(uid);
}

VersionList::Ptr Index::get(const QString& uid)
{
    VersionList::Ptr out = m_uids.value(uid, nullptr);
    if (!out) {
        out = std::make_shared<VersionList>(uid);
        m_uids[uid] = out;
        m_lists.append(out);
    }
    return out;
}

Version::Ptr Index::get(const QString& uid, const QString& version)
{
    auto list = get(uid);
    return list->getVersion(version);
}

void Index::parse(const QJsonObject& obj)
{
    parseIndex(obj, this);
}

void Index::merge(const std::shared_ptr<Index>& other)
{
    const QVector<VersionList::Ptr> lists = other->m_lists;
    // initial load, no need to merge
    if (m_lists.isEmpty()) {
        beginResetModel();
        m_lists = lists;
        for (int i = 0; i < lists.size(); ++i) {
            m_uids.insert(lists.at(i)->uid(), lists.at(i));
            connectVersionList(i, lists.at(i));
        }
        endResetModel();
    } else {
        for (const VersionList::Ptr& list : lists) {
            if (m_uids.contains(list->uid())) {
                m_uids[list->uid()]->mergeFromIndex(list);
            } else {
                beginInsertRows(QModelIndex(), m_lists.size(), m_lists.size());
                connectVersionList(m_lists.size(), list);
                m_lists.append(list);
                m_uids.insert(list->uid(), list);
                endInsertRows();
            }
        }
    }
}

void Index::connectVersionList(const int row, const VersionList::Ptr& list)
{
    connect(list.get(), &VersionList::nameChanged, this, [this, row] { emit dataChanged(index(row), index(row), { Qt::DisplayRole }); });
}

Task::Ptr Index::loadVersion(const QString& uid, const QString& version, Net::Mode mode, bool force)
{
    if (mode == Net::Mode::Offline) {
        return get(uid, version)->loadTask(mode);
    }

    auto versionList = get(uid);
    auto loadTask = makeShared<SequentialTask>(
        this, tr("Load meta for %1:%2", "This is for the task name that loads the meta index.").arg(uid, version));
    if (status() != BaseEntity::LoadStatus::Remote || force) {
        loadTask->addTask(this->loadTask(mode));
    }
    loadTask->addTask(versionList->loadTask(mode));
    loadTask->addTask(versionList->getVersion(version)->loadTask(mode));
    return loadTask;
}

Version::Ptr Index::getLoadedVersion(const QString& uid, const QString& version)
{
    QEventLoop ev;
    auto task = loadVersion(uid, version);
    QObject::connect(task.get(), &Task::finished, &ev, &QEventLoop::quit);
    task->start();
    ev.exec();
    return get(uid, version);
}
}  // namespace Meta
