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
	QVector<int> rowHeights;
	int firstRow;

	void update();

	void drawHeader(QPainter *painter, const int y);
	int totalHeight() const;
	int headerHeight() const;
	int contentHeight() const;
	int numRows() const;
	int top() const;

	enum HitResult
	{
		NoHit = 0x0,
		TextHit = 0x1,
		CheckboxHit = 0x2,
		HeaderHit = 0x4,
		BodyHit = 0x8
	};
	Q_DECLARE_FLAGS(HitResults, HitResult)

	HitResults pointIntersect (const QPoint &pos) const;

	QList<QModelIndex> items() const;
	int numItems() const;
	QModelIndex firstItem() const;
	QModelIndex lastItem() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Group::HitResults)
