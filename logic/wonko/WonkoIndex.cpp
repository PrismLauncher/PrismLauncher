/* Copyright 2015 MultiMC Contributors
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

#include "WonkoIndex.h"

#include "WonkoVersionList.h"
#include "tasks/BaseWonkoEntityLocalLoadTask.h"
#include "tasks/BaseWonkoEntityRemoteLoadTask.h"
#include "format/WonkoFormat.h"

WonkoIndex::WonkoIndex(QObject *parent)
	: QAbstractListModel(parent)
{
}
WonkoIndex::WonkoIndex(const QVector<WonkoVersionListPtr> &lists, QObject *parent)
	: QAbstractListModel(parent), m_lists(lists)
{
	for (int i = 0; i < m_lists.size(); ++i)
	{
		m_uids.insert(m_lists.at(i)->uid(), m_lists.at(i));
		connectVersionList(i, m_lists.at(i));
	}
}

QVariant WonkoIndex::data(const QModelIndex &index, int role) const
{
	if (index.parent().isValid() || index.row() < 0 || index.row() >= m_lists.size())
	{
		return QVariant();
	}

	WonkoVersionListPtr list = m_lists.at(index.row());
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
int WonkoIndex::rowCount(const QModelIndex &parent) const
{
	return m_lists.size();
}
int WonkoIndex::columnCount(const QModelIndex &parent) const
{
	return 1;
}
QVariant WonkoIndex::headerData(int section, Qt::Orientation orientation, int role) const
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

std::unique_ptr<Task> WonkoIndex::remoteUpdateTask()
{
	return std::unique_ptr<WonkoIndexRemoteLoadTask>(new WonkoIndexRemoteLoadTask(this, this));
}
std::unique_ptr<Task> WonkoIndex::localUpdateTask()
{
	return std::unique_ptr<WonkoIndexLocalLoadTask>(new WonkoIndexLocalLoadTask(this, this));
}

QJsonObject WonkoIndex::serialized() const
{
	return WonkoFormat::serializeIndex(this);
}

bool WonkoIndex::hasUid(const QString &uid) const
{
	return m_uids.contains(uid);
}
WonkoVersionListPtr WonkoIndex::getList(const QString &uid) const
{
	return m_uids.value(uid, nullptr);
}
WonkoVersionListPtr WonkoIndex::getListGuaranteed(const QString &uid) const
{
	return m_uids.value(uid, std::make_shared<WonkoVersionList>(uid));
}

void WonkoIndex::merge(const Ptr &other)
{
	const QVector<WonkoVersionListPtr> lists = std::dynamic_pointer_cast<WonkoIndex>(other)->m_lists;
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
		for (const WonkoVersionListPtr &list : lists)
		{
			if (m_uids.contains(list->uid()))
			{
				m_uids[list->uid()]->merge(list);
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

void WonkoIndex::connectVersionList(const int row, const WonkoVersionListPtr &list)
{
	connect(list.get(), &WonkoVersionList::nameChanged, this, [this, row]()
	{
		emit dataChanged(index(row), index(row), QVector<int>() << Qt::DisplayRole);
	});
}
