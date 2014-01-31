#include "Group.h"

#include <QModelIndex>
#include <QPainter>
#include <QtMath>

#include "GroupView.h"

Group::Group(const QString &text, GroupView *view) : view(view), text(text), collapsed(false)
{
}
Group::Group(const Group *other)
	: view(other->view), text(other->text), collapsed(other->collapsed),
	  iconRect(other->iconRect), textRect(other->textRect)
{
}

void Group::update()
{
	firstRow = firstItem().row();

	rowHeights = QVector<int>(numRows());
	for (int i = 0; i < numRows(); ++i)
	{
		rowHeights[i] = view->categoryRowHeight(
			view->model()->index(i * view->itemsPerRow() + firstRow, 0));
	}
}

void Group::drawHeader(QPainter *painter, const int y)
{
	painter->save();

	int height = headerHeight() - 4;
	int collapseSize = height;

	// the icon
	iconRect = QRect(view->m_rightMargin + 2, 2 + y, collapseSize, collapseSize);
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
	textRect = QRect(iconRect.right() + 4, y, textWidth, headerHeight());
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
	return headerHeight() + 5 + contentHeight();
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

int Group::top() const
{
	int res = 0;
	const QList<Group *> cats = view->m_categories;
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
		if (index.data(CategorizedViewRoles::CategoryRole).toString() == text)
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
