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
Index::Index(const QVector<VersionList::Ptr> &lists, QObject *parent)
    : QAbstractListModel(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists(lists)
{
    for (int i = 0; i < hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size(); ++i)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.insert(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.at(i)->uid(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.at(i));
        connectVersionList(i, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.at(i));
    }
}

QVariant Index::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.row() < 0 || index.row() >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size())
    {
        return QVariant();
    }

    VersionList::Ptr list = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.at(index.row());
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
    return parent.isValid() ? 0 : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size();
}
int Index::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
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
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.contains(uid);
}

VersionList::Ptr Index::get(const QString &uid)
{
    VersionList::Ptr out = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.value(uid, nullptr);
    if(!out)
    {
        out = std::make_shared<VersionList>(uid);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids[uid] = out;
    }
    return out;
}

Version::Ptr Index::get(const QString &uid, const QString &version)
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
    const QVector<VersionList::Ptr> lists = std::dynamic_pointer_cast<Index>(other)->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists;
    // initial load, no need to merge
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.isEmpty())
    {
        beginResetModel();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists = lists;
        for (int i = 0; i < lists.size(); ++i)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.insert(lists.at(i)->uid(), lists.at(i));
            connectVersionList(i, lists.at(i));
        }
        endResetModel();
    }
    else
    {
        for (const VersionList::Ptr &list : lists)
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.contains(list->uid()))
            {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids[list->uid()]->mergeFromIndex(list);
            }
            else
            {
                beginInsertRows(QModelIndex(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size());
                connectVersionList(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.size(), list);
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lists.append(list);
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uids.insert(list->uid(), list);
                endInsertRows();
            }
        }
    }
}

void Index::connectVersionList(const int row, const VersionList::Ptr &list)
{
    connect(list.get(), &VersionList::nameChanged, this, [this, row]()
    {
        emit dataChanged(index(row), index(row), QVector<int>() << Qt::DisplayRole);
    });
}
}
