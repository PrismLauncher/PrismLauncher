#pragma once

#include <QString>
#include <QRect>
#include <QVector>
#include <QStyleOption>

class GroupView;
class QPainter;
class QModelIndex;

struct Group
{
/* constructors */
	Group(const QString &text, GroupView *view);
	Group(const Group *other);

/* data */
	GroupView *view = nullptr;
	QString text;
	bool collapsed = false;
	QVector<int> rowHeights;
	int firstItemIndex = 0;
	int m_verticalPosition = 0;

/* logic */
	/// do stuff. and things. TODO: redo.
	void update();

	/// draw the header at y-position.
	void drawHeader(QPainter *painter, const QStyleOptionViewItem &option);

	/// height of the group, in total. includes a small bit of padding.
	int totalHeight() const;

	/// height of the group header, in pixels
	int headerHeight() const;

	/// height of the group content, in pixels
	int contentHeight() const;

	/// the number of visual rows this group has
	int numRows() const;

	/// the height at which this group starts, in pixels
	int verticalPosition() const;

	enum HitResult
	{
		NoHit = 0x0,
		TextHit = 0x1,
		CheckboxHit = 0x2,
		HeaderHit = 0x4,
		BodyHit = 0x8
	};
	Q_DECLARE_FLAGS(HitResults, HitResult)

	/// shoot! BANG! what did we hit?
	HitResults hitScan (const QPoint &pos) const;

	/// super derpy thing.
	QList<QModelIndex> items() const;
	/// I don't even
	int numItems() const;
	QModelIndex firstItem() const;
	QModelIndex lastItem() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Group::HitResults)
