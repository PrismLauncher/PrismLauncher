#include "Group.h"

#include <QModelIndex>
#include <QPainter>
#include <QtMath>

#include "GroupView.h"

Group::Group(const QString &text, GroupView *view) : view(view), text(text), collapsed(false)
{
}

Group::Group(const Group *other)
	: view(other->view), text(other->text), collapsed(other->collapsed)
{
}

void Group::update()
{
	firstItemIndex = firstItem().row();

	rowHeights = QVector<int>(numRows());
	for (int i = 0; i < numRows(); ++i)
	{
		rowHeights[i] = view->categoryRowHeight(
			view->model()->index(i * view->itemsPerRow() + firstItemIndex, 0));
	}
}

Group::HitResults Group::hitScan(const QPoint &pos) const
{
	Group::HitResults results = Group::NoHit;
	int y_start = verticalPosition();
	int body_start = y_start + headerHeight();
	int body_end = body_start + contentHeight() + 5; // FIXME: wtf is this 5?
	int y = pos.y();
	// int x = pos.x();
	if(y < y_start)
	{
		results = Group::NoHit;
	}
	else if(y < body_start)
	{
		results = Group::HeaderHit;
		int collapseSize = headerHeight() - 4;

		// the icon
		QRect iconRect = QRect(view->m_leftMargin + 2, 2 + y_start, collapseSize, collapseSize);
		if(iconRect.contains(pos))
		{
			results |= Group::CheckboxHit;
		}
	}
	else if (y < body_end)
	{
		results |= Group::BodyHit;
	}
	return results;
}

void Group::drawHeader(QPainter *painter, const int y)
{
	painter->save();

	int height = headerHeight() - 4;
	int collapseSize = height;

	// the icon
	QRect iconRect = QRect(view->m_leftMargin + 2, 2 + y, collapseSize, collapseSize);
	painter->setPen(QPen(Qt::black, 1));
	painter->drawRect(iconRect);
	static const int margin = 2;
	QRect iconSubrect = iconRect.adjusted(margin, margin, -margin, -margin);
	int midX = iconSubrect.center().x();
	int midY = iconSubrect.center().y();
	if (collapsed)
	{
		painter->drawLine(midX, iconSubrect.top(), midX, iconSubrect.bottom());
	}
	painter->drawLine(iconSubrect.left(), midY, iconSubrect.right(), midY);

	// the text
	int textWidth = painter->fontMetrics().width(text);
	QRect textRect = QRect(iconRect.right() + 4, y, textWidth, headerHeight());
	painter->setBrush(view->viewOptions().palette.text());
	view->style()->drawItemText(painter, textRect, Qt::AlignHCenter | Qt::AlignVCenter,
								view->viewport()->palette(), true, text);

	// the line
	painter->drawLine(textRect.right() + 4, y + headerHeight() / 2,
					  view->contentWidth() - view->m_rightMargin, y + headerHeight() / 2);

	painter->restore();
}

int Group::totalHeight() const
{
	return headerHeight() + 5 + contentHeight(); // FIXME: wtf is that '5'?
}

int Group::headerHeight() const
{
	return view->viewport()->fontMetrics().height() + 4;
}

int Group::contentHeight() const
{
	if (collapsed)
	{
		return 0;
	}
	int result = 0;
	for (int i = 0; i < rowHeights.size(); ++i)
	{
		result += rowHeights[i];
	}
	return result;
}

int Group::numRows() const
{
	return qMax(1, qCeil((qreal)numItems() / (qreal)view->itemsPerRow()));
}

int Group::verticalPosition() const
{
	int res = 0;
	const QList<Group *> cats = view->m_groups;
	for (int i = 0; i < cats.size(); ++i)
	{
		if (cats.at(i) == this)
		{
			break;
		}
		res += cats.at(i)->totalHeight() + view->m_categoryMargin;
	}
	return res;
}

QList<QModelIndex> Group::items() const
{
	QList<QModelIndex> indices;
	for (int i = 0; i < view->model()->rowCount(); ++i)
	{
		const QModelIndex index = view->model()->index(i, 0);
		if (index.data(GroupViewRoles::GroupRole).toString() == text)
		{
			indices.append(index);
		}
	}
	return indices;
}

int Group::numItems() const
{
	return items().size();
}

QModelIndex Group::firstItem() const
{
	QList<QModelIndex> indices = items();
	return indices.isEmpty() ? QModelIndex() : indices.first();
}

QModelIndex Group::lastItem() const
{
	QList<QModelIndex> indices = items();
	return indices.isEmpty() ? QModelIndex() : indices.last();
}
