/* Copyright 2013-2014 MultiMC Contributors
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

#include "EnabledItemFilter.h"

EnabledItemFilter::EnabledItemFilter(QObject *parent) : QSortFilterProxyModel(parent)
{
}

void EnabledItemFilter::setActive(bool active)
{
	m_active = active;
	invalidateFilter();
}

bool EnabledItemFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	if (!m_active)
		return true;
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	if (sourceModel()->flags(index) & Qt::ItemIsEnabled)
	{
		return true;
	}
	return false;
}

bool EnabledItemFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	return QSortFilterProxyModel::lessThan(left, right);
}
