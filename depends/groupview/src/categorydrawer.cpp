/**
  * This file is part of the KDE project
  * Copyright (C) 2007, 2009 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "categorydrawer.h"

#include <QPainter>
#include <QStyleOption>
#include <QApplication>

#include <categorizedview.h>
#include <categorizedsortfilterproxymodel.h>

#define HORIZONTAL_HINT 3

class KCategoryDrawer::Private
{
public:
	Private(KCategorizedView *view)
		: view(view)
		, leftMargin(0)
		, rightMargin(0)
    {
    }

    ~Private()
    {
    }
    int leftMargin;
    int rightMargin;
    KCategorizedView *view;
};

KCategoryDrawer::KCategoryDrawer(KCategorizedView *view)
    : QObject(view)
    , d(new Private(view))
{
	setLeftMargin(2);
	setRightMargin(2);
}

KCategoryDrawer::~KCategoryDrawer()
{
    delete d;
}


void KCategoryDrawer::drawCategory(const QModelIndex &index,
                                   int /*sortRole*/,
                                   const QStyleOption &option,
                                   QPainter *painter) const
{
    painter->setRenderHint(QPainter::Antialiasing);

    const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
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

    //BEGIN: text
    {
        QRect textRect(option.rect);
        textRect.setTop(textRect.top() + 7);
        textRect.setLeft(textRect.left() + 7);
        textRect.setHeight(fontMetrics.height());
        textRect.setRight(textRect.right() - 7);

        painter->save();
        painter->setFont(font);
        QColor penColor(option.palette.text().color());
        penColor.setAlphaF(0.6);
        painter->setPen(penColor);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, category);
        painter->restore();
    }
    //END: text
}

int KCategoryDrawer::categoryHeight(const QModelIndex &index, const QStyleOption &option) const
{
    Q_UNUSED(index);
    Q_UNUSED(option)

    QFont font(QApplication::font());
    font.setBold(true);
    QFontMetrics fontMetrics(font);

    const int height = fontMetrics.height() + 1 /* 1 pixel-width gradient */
                                            + 11 /* top and bottom separation */;
    return height;
}

int KCategoryDrawer::leftMargin() const
{
    return d->leftMargin;
}

void KCategoryDrawer::setLeftMargin(int leftMargin)
{
    d->leftMargin = leftMargin;
}

int KCategoryDrawer::rightMargin() const
{
    return d->rightMargin;
}

void KCategoryDrawer::setRightMargin(int rightMargin)
{
    d->rightMargin = rightMargin;
}

KCategoryDrawer &KCategoryDrawer::operator=(const KCategoryDrawer &cd)
{
    d->leftMargin = cd.d->leftMargin;
    d->rightMargin = cd.d->rightMargin;
	d->view = cd.d->view;
    return *this;
}

KCategorizedView *KCategoryDrawer::view() const
{
    return d->view;
}

void KCategoryDrawer::mouseButtonPressed(const QModelIndex&, const QRect&, QMouseEvent *event)
{
    event->ignore();
}

void KCategoryDrawer::mouseButtonReleased(const QModelIndex&, const QRect&, QMouseEvent *event)
{
    event->ignore();
}

void KCategoryDrawer::mouseMoved(const QModelIndex&, const QRect&, QMouseEvent *event)
{
    event->ignore();
}

void KCategoryDrawer::mouseButtonDoubleClicked(const QModelIndex&, const QRect&, QMouseEvent *event)
{
    event->ignore();
}

void KCategoryDrawer::mouseLeft(const QModelIndex&, const QRect&)
{
}

#include "categorydrawer.moc"
