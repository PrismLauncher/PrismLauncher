#include "CategorizedProxyModel.h"

#include "CategorizedView.h"

CategorizedProxyModel::CategorizedProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}
bool CategorizedProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	return left.data(CategorizedView::CategoryRole).toString() < right.data(CategorizedView::CategoryRole).toString();
}
