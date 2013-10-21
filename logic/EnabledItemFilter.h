#pragma once
#include <QSortFilterProxyModel>

class EnabledItemFilter : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	EnabledItemFilter(QObject *parent = 0);
	void setActive(bool active);
	
protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
private:
	bool m_active = false;
};