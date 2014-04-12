/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceDelegate.h"
#include <QPainter>
#include <QTextOption>
#include <QTextLayout>
#include <QApplication>
#include <QtMath>

#include "GroupView.h"
#include "logic/BaseInstance.h"
#include "logic/InstanceList.h"

QCache<QString, QPixmap> ListViewDelegate::m_pixmapCache;

// Origin: Qt
static void viewItemTextLayout(QTextLayout &textLayout, int lineWidth, qreal &height,
							   qreal &widthUsed)
{
	height = 0;
	widthUsed = 0;
	textLayout.beginLayout();
	QString str = textLayout.text();
	while (true)
	{
		QTextLine line = textLayout.createLine();
		if (!line.isValid())
			break;
		if (line.textLength() == 0)
			break;
		line.setLineWidth(lineWidth);
		line.setPosition(QPointF(0, height));
		height += line.height();
		widthUsed = qMax(widthUsed, line.naturalTextWidth());
	}
	textLayout.endLayout();
}

ListViewDelegate::ListViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void drawSelectionRect(QPainter *painter, const QStyleOptionViewItemV4 &option,
					   const QRect &rect)
{
	if ((option.state & QStyle::State_Selected))
		painter->fillRect(rect, option.palette.brush(QPalette::Highlight));
	else
	{
		QColor backgroundColor = option.palette.color(QPalette::Background);
		backgroundColor.setAlpha(160);
		painter->fillRect(rect, QBrush(backgroundColor));
	}
}

void drawFocusRect(QPainter *painter, const QStyleOptionViewItemV4 &option, const QRect &rect)
{
	if (!(option.state & QStyle::State_HasFocus))
		return;
	QStyleOptionFocusRect opt;
	opt.direction = option.direction;
	opt.fontMetrics = option.fontMetrics;
	opt.palette = option.palette;
	opt.rect = rect;
	// opt.state           = option.state | QStyle::State_KeyboardFocusChange |
	// QStyle::State_Item;
	auto col = option.state & QStyle::State_Selected ? QPalette::Highlight : QPalette::Base;
	opt.backgroundColor = option.palette.color(col);
	// Apparently some widget styles expect this hint to not be set
	painter->setRenderHint(QPainter::Antialiasing, false);

	QStyle *style = option.widget ? option.widget->style() : QApplication::style();

	style->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, painter, option.widget);

	painter->setRenderHint(QPainter::Antialiasing);
}

// TODO this can be made a lot prettier
void drawProgressOverlay(QPainter *painter, const QStyleOptionViewItemV4 &option,
						 const int value, const int maximum)
{
	if (maximum == 0 || value == maximum)
	{
		return;
	}

	painter->save();

	qreal percent = (qreal)value / (qreal)maximum;
	QColor color = option.palette.color(QPalette::Dark);
	color.setAlphaF(0.70f);
	painter->setBrush(color);
	painter->setPen(QPen(QBrush(), 0));
	painter->drawPie(option.rect, 90 * 16, -percent * 360 * 16);

	painter->restore();
}

void drawBadges(QPainter *painter, const QStyleOptionViewItemV4 &option, BaseInstance *instance)
{
	QList<QString> pixmaps;
	for (auto flag : instance->flags())
	{
		switch (flag)
		{
		case BaseInstance::VersionBrokenFlag:
			pixmaps.append("broken");
			break;
		default:
			break;
		}
	}

	// begin easter eggs
	if (instance->name().contains("btw", Qt::CaseInsensitive) ||
		instance->name().contains("better then wolves", Qt::CaseInsensitive) ||
		instance->name().contains("better than wolves", Qt::CaseInsensitive))
	{
		pixmaps.append("herobrine");
	}
	if (instance->name().contains("direwolf", Qt::CaseInsensitive))
	{
		pixmaps.append("enderman");
	}
	if (instance->name().contains("kitten", Qt::CaseInsensitive))
	{
		pixmaps.append("kitten");
	}
	if (instance->name().contains("derp", Qt::CaseInsensitive))
	{
		pixmaps.append("derp");
	}
	// end easter eggs

	static const int itemSide = 24;
	static const int spacing = 1;
	const int itemsPerRow = qMax(1, qFloor(double(option.rect.width() + spacing) / double(itemSide + spacing)));
	const int rows = qCeil((double)pixmaps.size() / (double)itemsPerRow);
	QListIterator<QString> it(pixmaps);
	painter->translate(option.rect.topLeft());
	for (int y = 0; y < rows; ++y)
	{
		for (int x = 0; x < itemsPerRow; ++x)
		{
			if (!it.hasNext())
			{
				return;
			}
			const QPixmap pixmap = ListViewDelegate::requestPixmap(it.next()).scaled(
				itemSide, itemSide, Qt::KeepAspectRatio, Qt::FastTransformation);
			painter->drawPixmap(option.rect.width() - x * itemSide + qMax(x - 1, 0) * spacing - itemSide,
								y * itemSide + qMax(y - 1, 0) * spacing, itemSide, itemSide,
								pixmap);
		}
	}
	painter->translate(-option.rect.topLeft());
}

static QSize viewItemTextSize(const QStyleOptionViewItemV4 *option)
{
	QStyle *style = option->widget ? option->widget->style() : QApplication::style();
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	QTextLayout textLayout;
	textLayout.setTextOption(textOption);
	textLayout.setFont(option->font);
	textLayout.setText(option->text);
	const int textMargin =
		style->pixelMetric(QStyle::PM_FocusFrameHMargin, option, option->widget) + 1;
	QRect bounds(0, 0, 100 - 2 * textMargin, 600);
	qreal height = 0, widthUsed = 0;
	viewItemTextLayout(textLayout, bounds.width(), height, widthUsed);
	const QSize size(qCeil(widthUsed), qCeil(height));
	return QSize(size.width() + 2 * textMargin, size.height());
}

void ListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
							 const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);
	painter->save();
	painter->setClipRect(opt.rect);

	opt.features |= QStyleOptionViewItem::WrapText;
	opt.text = index.data().toString();
	opt.textElideMode = Qt::ElideRight;
	opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

	QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

	// const int iconSize =  style->pixelMetric(QStyle::PM_IconViewIconSize);
	const int iconSize = 48;
	QRect iconbox = opt.rect;
	const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) + 1;
	QRect textRect = opt.rect;
	QRect textHighlightRect = textRect;
	// clip the decoration on top, remove width padding
	textRect.adjust(textMargin, iconSize + textMargin + 5, -textMargin, 0);

	textHighlightRect.adjust(0, iconSize + 5, 0, 0);

	// draw background
	{
		// FIXME: unused
		// QSize textSize = viewItemTextSize ( &opt );
		QPalette::ColorGroup cg;
		QStyleOptionViewItemV4 opt2(opt);

		if ((opt.widget && opt.widget->isEnabled()) || (opt.state & QStyle::State_Enabled))
		{
			if (!(opt.state & QStyle::State_Active))
				cg = QPalette::Inactive;
			else
				cg = QPalette::Normal;
		}
		else
		{
			cg = QPalette::Disabled;
		}
		opt2.palette.setCurrentColorGroup(cg);

		// fill in background, if any

		if (opt.backgroundBrush.style() != Qt::NoBrush)
		{
			QPointF oldBO = painter->brushOrigin();
			painter->setBrushOrigin(opt.rect.topLeft());
			painter->fillRect(opt.rect, opt.backgroundBrush);
			painter->setBrushOrigin(oldBO);
		}

		drawSelectionRect(painter, opt2, textHighlightRect);

		/*
		if (opt.showDecorationSelected)
		{
			drawSelectionRect(painter, opt2, opt.rect);
			drawFocusRect(painter, opt2, opt.rect);
			// painter->fillRect ( opt.rect, opt.palette.brush ( cg, QPalette::Highlight ) );
		}
		else
		{

			// if ( opt.state & QStyle::State_Selected )
			{
				// QRect textRect = subElementRect ( QStyle::SE_ItemViewItemText,  opt,
				// opt.widget );
				// painter->fillRect ( textHighlightRect, opt.palette.brush ( cg,
				// QPalette::Highlight ) );
				drawSelectionRect(painter, opt2, textHighlightRect);
				drawFocusRect(painter, opt2, textHighlightRect);
			}
		}
		*/
	}

	// draw the icon
	{
		QIcon::Mode mode = QIcon::Normal;
		if (!(opt.state & QStyle::State_Enabled))
			mode = QIcon::Disabled;
		else if (opt.state & QStyle::State_Selected)
			mode = QIcon::Selected;
		QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

		iconbox.setHeight(iconSize);
		opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
	}
	// set the text colors
	QPalette::ColorGroup cg =
		opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
		cg = QPalette::Inactive;
	if (opt.state & QStyle::State_Selected)
	{
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	}
	else
	{
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the text
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	textOption.setTextDirection(opt.direction);
	textOption.setAlignment(QStyle::visualAlignment(opt.direction, opt.displayAlignment));
	QTextLayout textLayout;
	textLayout.setTextOption(textOption);
	textLayout.setFont(opt.font);
	textLayout.setText(opt.text);

	qreal width, height;
	viewItemTextLayout(textLayout, textRect.width(), height, width);

	const int lineCount = textLayout.lineCount();

	const QRect layoutRect = QStyle::alignedRect(
		opt.direction, opt.displayAlignment, QSize(textRect.width(), int(height)), textRect);
	const QPointF position = layoutRect.topLeft();
	for (int i = 0; i < lineCount; ++i)
	{
		const QTextLine line = textLayout.lineAt(i);
		line.draw(painter, position);
	}

	// FIXME: this really has no business of being here. Make generic.
	auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole)
			.value<void *>();
	if (instance)
	{
		drawBadges(painter, opt, instance);
	}

	drawProgressOverlay(painter, opt, index.data(GroupViewRoles::ProgressValueRole).toInt(),
						index.data(GroupViewRoles::ProgressMaximumRole).toInt());

	painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem &option,
								 const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);
	opt.features |= QStyleOptionViewItem::WrapText;
	opt.text = index.data().toString();
	opt.textElideMode = Qt::ElideRight;
	opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

	QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
	const int textMargin =
		style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, opt.widget) + 1;
	int height = 48 + textMargin * 2 + 5; // TODO: turn constants into variables
	QSize szz = viewItemTextSize(&opt);
	height += szz.height();
	// FIXME: maybe the icon items could scale and keep proportions?
	QSize sz(100, height);
	return sz;
}

QPixmap ListViewDelegate::requestPixmap(const QString &key)
{
	if (!m_pixmapCache.contains(key))
	{
		m_pixmapCache.insert(key, new QPixmap(":/icons/badges/" + key + ".png"));
	}
	return *m_pixmapCache.object(key);
}
