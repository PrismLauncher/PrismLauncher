#pragma once

#include <QString>
#include <QRect>
#include <QVector>
#include <QStyleOption>

class GroupView;
class QPainter;
class QModelIndex;

struct VisualRow
{
	QList<QModelIndex> items;
	int height = 0;
	int top = 0;
	inline int size() const
	{
		return items.size();
	}
	inline QModelIndex &operator[](int i)
	{
		return items[i];
	}
};

struct VisualGroup
{
/* constructors */
	VisualGroup(const QString &text, GroupView *view);
	VisualGroup(const VisualGroup *other);

/* data */
	GroupView *view = nullptr;
	QString text;
	bool collapsed = false;
	QVector<VisualRow> rows;
	int firstItemIndex = 0;
	int m_verticalPosition = 0;

/* logic */
	/// update the internal list of items and flow them into the rows.
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

	/// actually calculate the above value
	int calculateNumRows() const;

	/// the height at which this group starts, in pixels
	int verticalPosition() const;

	/// relative geometry - top of the row of the given item
	int rowTopOf(const QModelIndex &index) const;

	/// height of the row of the given item
	int rowHeightOf(const QModelIndex &index) const;

	/// x/y position of the given item inside the group (in items!)
	QPair<int, int> positionOf(const QModelIndex &index) const;

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

	QList<QModelIndex> items() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VisualGroup::HitResults)
