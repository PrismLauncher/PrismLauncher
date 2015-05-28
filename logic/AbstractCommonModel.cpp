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

#include "AbstractCommonModel.h"

BaseAbstractCommonModel::BaseAbstractCommonModel(const Qt::Orientation orientation, QObject *parent)
	: QAbstractListModel(parent), m_orientation(orientation)
{
}

int BaseAbstractCommonModel::rowCount(const QModelIndex &parent) const
{
	return m_orientation == Qt::Horizontal ? entryCount() : size();
}
int BaseAbstractCommonModel::columnCount(const QModelIndex &parent) const
{
	return m_orientation == Qt::Horizontal ? size() : entryCount();
}
QVariant BaseAbstractCommonModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent()))
	{
		return QVariant();
	}
	const int i = m_orientation == Qt::Horizontal ? index.column() : index.row();
	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	return formatData(i, role, get(i, entry, role));
}
QVariant BaseAbstractCommonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != m_orientation && role == Qt::DisplayRole)
	{
		return entryTitle(section);
	}
	else
	{
		return QVariant();
	}
}
bool BaseAbstractCommonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	const int i = m_orientation == Qt::Horizontal ? index.column() : index.row();
	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	const bool result = set(i, entry, role, sanetizeData(i, role, value));
	if (result)
	{
		emit dataChanged(index, index, QVector<int>() << role);
	}
	return result;
}
Qt::ItemFlags BaseAbstractCommonModel::flags(const QModelIndex &index) const
{
	if (!hasIndex(index.row(), index.column(), index.parent()))
	{
		return Qt::NoItemFlags;
	}

	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	if (canSet(entry))
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}
	else
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void BaseAbstractCommonModel::notifyAboutToAddObject(const int at)
{
	if (m_orientation == Qt::Horizontal)
	{
		beginInsertColumns(QModelIndex(), at, at);
	}
	else
	{
		beginInsertRows(QModelIndex(), at, at);
	}
}
void BaseAbstractCommonModel::notifyObjectAdded()
{
	if (m_orientation == Qt::Horizontal)
	{
		endInsertColumns();
	}
	else
	{
		endInsertRows();
	}
}
void BaseAbstractCommonModel::notifyAboutToRemoveObject(const int at)
{
	if (m_orientation == Qt::Horizontal)
	{
		beginRemoveColumns(QModelIndex(), at, at);
	}
	else
	{
		beginRemoveRows(QModelIndex(), at, at);
	}
}
void BaseAbstractCommonModel::notifyObjectRemoved()
{
	if (m_orientation == Qt::Horizontal)
	{
		endRemoveColumns();
	}
	else
	{
		endRemoveRows();
	}
}

void BaseAbstractCommonModel::notifyBeginReset()
{
	beginResetModel();
}
void BaseAbstractCommonModel::notifyEndReset()
{
	endResetModel();
}
