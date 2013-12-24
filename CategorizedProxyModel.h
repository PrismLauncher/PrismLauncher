#ifndef CATEGORIZEDPROXYMODEL_H
#define CATEGORIZEDPROXYMODEL_H

#include <QSortFilterProxyModel>

class CategorizedProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	CategorizedProxyModel(QObject *parent = 0);

protected:
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};


#endif // CATEGORIZEDPROXYMODEL_H
