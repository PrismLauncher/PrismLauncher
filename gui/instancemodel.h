#pragma once

#include <QAbstractListModel>
#include "kcategorizedsortfilterproxymodel.h"
#include "instancelist.h"
#include <QIcon>

class InstanceModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum AdditionalRoles
	{
		InstancePointerRole = 0x34B1CB48 ///< Return pointer to real instance
	};
	explicit InstanceModel ( const InstanceList& instances,
	                         QObject *parent = 0 );

	QModelIndex  index ( int row, int column = 0,
	                     const QModelIndex& parent = QModelIndex() ) const;
	int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
	QVariant data ( const QModelIndex& index, int role ) const;
	Qt::ItemFlags flags ( const QModelIndex& index ) const;

public slots:
	void onInstanceAdded(int index);
	void onInstanceChanged(int index);
	void onInvalidated();

private:
	const InstanceList* m_instances;
	QIcon cachedIcon;
	int currentInstancesNumber;
};

class InstanceProxyModel : public KCategorizedSortFilterProxyModel
{
public:
	explicit InstanceProxyModel ( QObject *parent = 0 );

protected:
	virtual bool subSortLessThan ( const QModelIndex& left, const QModelIndex& right ) const;
};

