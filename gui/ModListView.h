#pragma once
#include <QTreeView>

class ModListView: public QTreeView
{
	Q_OBJECT
public:
	explicit ModListView ( QWidget* parent = 0 );
	virtual void setModel ( QAbstractItemModel* model );
};