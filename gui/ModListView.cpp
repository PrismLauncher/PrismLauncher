#include "ModListView.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QDrag>
#include <QRect>

ModListView::ModListView ( QWidget* parent )
	:QTreeView ( parent )
{
	setAllColumnsShowFocus ( true );
	setExpandsOnDoubleClick ( false );
	setRootIsDecorated ( false );
	setSortingEnabled ( false );
	setAlternatingRowColors ( true );
	setSelectionMode ( QAbstractItemView::SingleSelection );
	setHeaderHidden ( false );
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOn );
	setHorizontalScrollBarPolicy ( Qt::ScrollBarAsNeeded );
	setDropIndicatorShown(true);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::DropOnly);
	viewport()->setAcceptDrops(true);
}

void ModListView::setModel ( QAbstractItemModel* model )
{
	QTreeView::setModel ( model );
	auto head = header();
	head->setStretchLastSection(false);
	head->setSectionResizeMode(0, QHeaderView::Stretch);
	head->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	dropIndicatorPosition();
}
