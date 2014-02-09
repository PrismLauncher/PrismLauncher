#include "GroupedProxyModel.h"

#include "GroupView.h"

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
		return leftCategory < rightCategory;
	}
}

bool GroupedProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
	return left.row() < right.row();
}
