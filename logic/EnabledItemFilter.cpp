#include "EnabledItemFilter.h"

EnabledItemFilter::EnabledItemFilter(QObject* parent)
	:QSortFilterProxyModel(parent)
{
	
}

void EnabledItemFilter::setActive(bool active)
{
	m_active = active;
	invalidateFilter();
}

bool EnabledItemFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	if(!m_active)
		return true;
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	if(sourceModel()->flags(index) & Qt::ItemIsEnabled)
	{
		return true;
	}
	return false;
}

bool EnabledItemFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    return QSortFilterProxyModel::lessThan(left, right);
}
