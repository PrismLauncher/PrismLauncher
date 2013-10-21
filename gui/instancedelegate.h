#pragma once

#include <QStyledItemDelegate>

class ListViewDelegate : public QStyledItemDelegate
{
public:
	explicit ListViewDelegate ( QObject* parent = 0 );
protected:
	void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
	QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};
