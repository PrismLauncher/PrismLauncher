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

#include "VersionList.h"
#include "JsonFormat.h"

namespace Meta
{
Index::Index(QObject *parent)
    : QAbstractListModel(parent)
{
}
Index::Index(const QVector<VersionListPtr> &lists, QObject *parent)
    : QAbstractListModel(parent), m_lists(lists)
{
    for (int i = 0; i < m_lists.size(); ++i)
    {
        m_uids.insert(m_lists.at(i)->uid(), m_lists.at(i));
        connectVersionList(i, m_lists.at(i));
    }
}

QVariant Index::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.row() < 0 || index.row() >= m_lists.size())
    {
        return QVariant();
    }

    VersionListPtr list = m_lists.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case 0: return list->humanReadable();
        default: break;
        }
    case UidRole: return list->uid();
    case NameRole: return list->name();
    case ListPtrRole: return QVariant::fromValue(list);
    }
    return QVariant();
}
int Index::rowCount(const QModelIndex &parent) const
{
    return m_lists.size();
}
int Index::columnCount(const QModelIndex &parent) const
{
    return 1;
}
QVariant Index::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
    {
        return tr("Name");
    }
    else
    {
        return QVariant();
    }
}

bool Index::hasUid(const QString &uid) const
{
    return m_uids.contains(uid);
}

VersionListPtr Index::get(const QString &uid)
{
    VersionListPtr out = m_uids.value(uid, nullptr);
    if(!out)
    {
        out = std::make_shared<VersionList>(uid);
        m_uids[uid] = out;
    }
    return out;
}

VersionPtr Index::get(const QString &uid, const QString &version)
{
    auto list = get(uid);
    return list->getVersion(version);
}

void Index::parse(const QJsonObject& obj)
{
    parseIndex(obj, this);
}

void Index::merge(const std::shared_ptr<Index> &other)
{
    const QVector<VersionListPtr> lists = std::dynamic_pointer_cast<Index>(other)->m_lists;
    // initial load, no need to merge
    if (m_lists.isEmpty())
    {
        beginResetModel();
        m_lists = lists;
        for (int i = 0; i < lists.size(); ++i)
        {
            m_uids.insert(lists.at(i)->uid(), lists.at(i));
            connectVersionList(i, lists.at(i));
        }
        endResetModel();
    }
    else
    {
        for (const VersionListPtr &list : lists)
        {
            if (m_uids.contains(list->uid()))
            {
                m_uids[list->uid()]->mergeFromIndex(list);
            }
            else
            {
                beginInsertRows(QModelIndex(), m_lists.size(), m_lists.size());
                connectVersionList(m_lists.size(), list);
                m_lists.append(list);
                m_uids.insert(list->uid(), list);
                endInsertRows();
            }
        }
    }
}

void Index::connectVersionList(const int row, const VersionListPtr &list)
{
    connect(list.get(), &VersionList::nameChanged, this, [this, row]()
    {
        emit dataChanged(index(row), index(row), QVector<int>() << Qt::DisplayRole);
    });
}
}
