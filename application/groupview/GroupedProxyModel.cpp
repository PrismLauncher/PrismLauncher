#include "GroupedProxyModel.h"

#include "GroupView.h"
#include <QDebug>

GroupedProxyModel::GroupedProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool GroupedProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	const QString leftCategory = left.data(GroupViewRoles::GroupRole).toString();
	const QString rightCategory = right.data(GroupViewRoles::GroupRole).toString();
	if (leftCategory == rightCategory)
	{
		return subSortLessThan(left, right);
	}
	else
	{
		// FIXME: real group sorting happens in GroupView::updateGeometries(), see LocaleString
		auto result = leftCategory.localeAwareCompare(rightCategory);
		if(result == 0)
		{
			return subSortLessThan(left, right);
		}
		return result < 0;
	}
}

bool GroupedProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
	return left.row() < right.row();
}
