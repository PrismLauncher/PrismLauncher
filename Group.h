#pragma once

#include <QString>
#include <QRect>
#include <QVector>

class GroupView;
class QPainter;
class QModelIndex;

struct Group
{
	Group(const QString &text, GroupView *view);
	Group(const Group *other);
	GroupView *view;
	QString text;
	bool collapsed;
	QRect iconRect;
	QRect textRect;
	QVector<int> rowHeights;
	int firstRow;

	void update();

	void drawHeader(QPainter *painter, const int y);
	int totalHeight() const;
	int headerHeight() const;
	int contentHeight() const;
	int numRows() const;
	int top() const;

	QList<QModelIndex> items() const;
	int numItems() const;
	QModelIndex firstItem() const;
	QModelIndex lastItem() const;
};
