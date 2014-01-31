#pragma once

#include <QSortFilterProxyModel>

class GroupedProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	GroupedProxyModel(QObject *parent = 0);

protected:
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};
