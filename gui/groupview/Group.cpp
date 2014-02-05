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
	if (y < y_start)
	{
		results = Group::NoHit;
	}
	else if (y < body_start)
	{
		results = Group::HeaderHit;
		int collapseSize = headerHeight() - 4;

		// the icon
		QRect iconRect = QRect(view->m_leftMargin + 2, 2 + y_start, collapseSize, collapseSize);
		if (iconRect.contains(pos))
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

void Group::drawHeader(QPainter *painter, const QStyleOptionViewItem &option, const int y)
{
	QStyleOptionViewItemV4 opt = option;
	painter->save();

	static const int margin = 2;
	static const int spacing = 10;
	int height = headerHeight();
	int text_height = height - 2 * margin;

	// set the text colors
	QPalette::ColorGroup cg = QPalette::Normal;
	painter->setPen(opt.palette.color(cg, QPalette::Text));

	// set up geometry
	QRect iconRect = QRect(view->m_leftMargin + margin, y + margin, text_height - 1, text_height - 1);
	QRect iconSubrect = iconRect.adjusted(margin, margin, -margin, -margin);
	QRect smallRect = iconSubrect.adjusted(margin, margin, -margin, -margin);
	int midX = iconSubrect.center().x();
	int midY = iconSubrect.center().y();

	// checkboxy thingy
	{
		painter->drawRect(iconSubrect);

		painter->setBrush(opt.palette.text());
		painter->drawRect(smallRect.x(), midY, smallRect.width(), 2);
		if(collapsed)
		{
			painter->drawRect(midX, smallRect.y(), 2, smallRect.height());
		}
	}

	int x_left = iconRect.right();

	if(text.length())
	{
		// the text
		int text_width = painter->fontMetrics().width(text);
		QRect textRect = QRect(x_left + spacing, y + margin, text_width, text_height);
		x_left = textRect.right();
		view->style()->drawItemText(painter, textRect, Qt::AlignHCenter | Qt::AlignVCenter,
									opt.palette, true, text);
	}
	// the line
	painter->drawLine(x_left + spacing, midY + 1, view->contentWidth() - view->m_rightMargin,
					  midY + 1);

	painter->restore();
}

int Group::totalHeight() const
{
	return headerHeight() + 5 + contentHeight(); // FIXME: wtf is that '5'?
}

int Group::headerHeight() const
{
	int raw = view->viewport()->fontMetrics().height() + 4;
	// add english. maybe. depends on font height.
	if(raw % 2 == 0)
		raw++;
	return std::min( raw , 25 );
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
