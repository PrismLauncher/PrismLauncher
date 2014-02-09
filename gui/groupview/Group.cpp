#include "Group.h"

#include <QModelIndex>
#include <QPainter>
#include <QtMath>
#include <QApplication>

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

void Group::drawHeader(QPainter *painter, const QStyleOptionViewItem &option)
{
	painter->setRenderHint(QPainter::Antialiasing);

	const QRect optRect = option.rect;
	QFont font(QApplication::font());
	font.setBold(true);
	const QFontMetrics fontMetrics = QFontMetrics(font);

	QColor outlineColor = option.palette.text().color();
	outlineColor.setAlphaF(0.35);

	//BEGIN: top left corner
	{
		painter->save();
		painter->setPen(outlineColor);
		const QPointF topLeft(optRect.topLeft());
		QRectF arc(topLeft, QSizeF(4, 4));
		arc.translate(0.5, 0.5);
		painter->drawArc(arc, 1440, 1440);
		painter->restore();
	}
	//END: top left corner

	//BEGIN: left vertical line
	{
		QPoint start(optRect.topLeft());
		start.ry() += 3;
		QPoint verticalGradBottom(optRect.topLeft());
		verticalGradBottom.ry() += fontMetrics.height() + 5;
		QLinearGradient gradient(start, verticalGradBottom);
		gradient.setColorAt(0, outlineColor);
		gradient.setColorAt(1, Qt::transparent);
		painter->fillRect(QRect(start, QSize(1, fontMetrics.height() + 5)), gradient);
	}
	//END: left vertical line

	//BEGIN: horizontal line
	{
		QPoint start(optRect.topLeft());
		start.rx() += 3;
		QPoint horizontalGradTop(optRect.topLeft());
		horizontalGradTop.rx() += optRect.width() - 6;
		painter->fillRect(QRect(start, QSize(optRect.width() - 6, 1)), outlineColor);
	}
	//END: horizontal line

	//BEGIN: top right corner
	{
		painter->save();
		painter->setPen(outlineColor);
		QPointF topRight(optRect.topRight());
		topRight.rx() -= 4;
		QRectF arc(topRight, QSizeF(4, 4));
		arc.translate(0.5, 0.5);
		painter->drawArc(arc, 0, 1440);
		painter->restore();
	}
	//END: top right corner

	//BEGIN: right vertical line
	{
		QPoint start(optRect.topRight());
		start.ry() += 3;
		QPoint verticalGradBottom(optRect.topRight());
		verticalGradBottom.ry() += fontMetrics.height() + 5;
		QLinearGradient gradient(start, verticalGradBottom);
		gradient.setColorAt(0, outlineColor);
		gradient.setColorAt(1, Qt::transparent);
		painter->fillRect(QRect(start, QSize(1, fontMetrics.height() + 5)), gradient);
	}
	//END: right vertical line

	//BEGIN: checkboxy thing
	{
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing, false);
		painter->setFont(font);
		QColor penColor(option.palette.text().color());
		penColor.setAlphaF(0.6);
		painter->setPen(penColor);
		QRect iconSubRect(option.rect);
		iconSubRect.setTop(iconSubRect.top() + 7);
		iconSubRect.setLeft(iconSubRect.left() + 7);

		int sizing = fontMetrics.height();
		int even = ( (sizing - 1) % 2 );

		iconSubRect.setHeight(sizing - even);
		iconSubRect.setWidth(sizing - even);
		painter->drawRect(iconSubRect);


		/*
		if(collapsed)
			painter->drawText(iconSubRect, Qt::AlignHCenter | Qt::AlignVCenter, "+");
		else
			painter->drawText(iconSubRect, Qt::AlignHCenter | Qt::AlignVCenter, "-");
		*/
		painter->setBrush(option.palette.text());
		painter->fillRect(iconSubRect.x(), iconSubRect.y() + iconSubRect.height() / 2,
						  iconSubRect.width(), 2, penColor);
		if (collapsed)
		{
			painter->fillRect(iconSubRect.x() + iconSubRect.width() / 2, iconSubRect.y(), 2,
							  iconSubRect.height(), penColor);
		}

		painter->restore();
	}
	//END: checkboxy thing

	//BEGIN: text
	{
		QRect textRect(option.rect);
		textRect.setTop(textRect.top() + 7);
		textRect.setLeft(textRect.left() + 7 + fontMetrics.height() + 7);
		textRect.setHeight(fontMetrics.height());
		textRect.setRight(textRect.right() - 7);

		painter->save();
		painter->setFont(font);
		QColor penColor(option.palette.text().color());
		penColor.setAlphaF(0.6);
		painter->setPen(penColor);
		painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
		painter->restore();
	}
	//END: text
}

int Group::totalHeight() const
{
	return headerHeight() + 5 + contentHeight(); // FIXME: wtf is that '5'?
}

int Group::headerHeight() const
{
	QFont font(QApplication::font());
    font.setBold(true);
    QFontMetrics fontMetrics(font);

    const int height = fontMetrics.height() + 1 /* 1 pixel-width gradient */
                                            + 11 /* top and bottom separation */;
    return height;
	/*
	int raw = view->viewport()->fontMetrics().height() + 4;
	// add english. maybe. depends on font height.
	if (raw % 2 == 0)
		raw++;
	return std::min(raw, 25);
	*/
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
	return m_verticalPosition;
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
