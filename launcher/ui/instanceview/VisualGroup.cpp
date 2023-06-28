// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "VisualGroup.h"

#include <QModelIndex>
#include <QPainter>
#include <QtMath>
#include <QApplication>
#include <QDebug>

#include "InstanceView.h"

VisualGroup::VisualGroup(const QString &text, InstanceView *view) : view(view), text(text), collapsed(false)
{
}

VisualGroup::VisualGroup(const VisualGroup *other)
    : view(other->view), text(other->text), collapsed(other->collapsed)
{
}

void VisualGroup::update()
{
    auto temp_items = items();
    auto itemsPerRow = view->itemsPerRow();

    int numRows = qMax(1, qCeil((qreal)temp_items.size() / (qreal)itemsPerRow));
    rows = QVector<VisualRow>(numRows);

    int maxRowHeight = 0;
    int positionInRow = 0;
    int currentRow = 0;
    int offsetFromTop = 0;
    for (auto item: temp_items)
    {
        if(positionInRow == itemsPerRow)
        {
            rows[currentRow].height = maxRowHeight;
            rows[currentRow].top = offsetFromTop;
            currentRow ++;
            offsetFromTop += maxRowHeight + 5;
            positionInRow = 0;
            maxRowHeight = 0;
        }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QStyleOptionViewItem viewItemOption;
        view->initViewItemOption(&viewItemOption);
#else
        QStyleOptionViewItem viewItemOption = view->viewOptions();
#endif

        auto itemHeight = view->itemDelegate()->sizeHint(viewItemOption, item).height();
        if(itemHeight > maxRowHeight)
        {
            maxRowHeight = itemHeight;
        }
        rows[currentRow].items.append(item);
        positionInRow++;
    }
    rows[currentRow].height = maxRowHeight;
    rows[currentRow].top = offsetFromTop;
}

QPair<int, int> VisualGroup::positionOf(const QModelIndex &index) const
{
    int y = 0;
    for (auto & row: rows)
    {
        for(auto x = 0; x < row.items.size(); x++)
        {
            if(row.items[x] == index)
            {
                return qMakePair(x,y);
            }
        }
        y++;
    }
    qWarning() << "Item" << index.row() << index.data(Qt::DisplayRole).toString() << "not found in visual group" << text;
    return qMakePair(0, 0);
}

int VisualGroup::rowTopOf(const QModelIndex &index) const
{
    auto position = positionOf(index);
    return rows[position.second].top;
}

int VisualGroup::rowHeightOf(const QModelIndex &index) const
{
    auto position = positionOf(index);
    return rows[position.second].height;
}

VisualGroup::HitResults VisualGroup::hitScan(const QPoint &pos) const
{
    VisualGroup::HitResults results = VisualGroup::NoHit;
    int y_start = verticalPosition();
    int body_start = y_start + headerHeight();
    int body_end = body_start + contentHeight() + 5; // FIXME: wtf is this 5?
    int y = pos.y();
    // int x = pos.x();
    if (y < y_start)
    {
        results = VisualGroup::NoHit;
    }
    else if (y < body_start)
    {
        results = VisualGroup::HeaderHit;
        int collapseSize = headerHeight() - 4;

        // the icon
        QRect iconRect = QRect(view->m_leftMargin + 2, 2 + y_start, collapseSize, collapseSize);
        if (iconRect.contains(pos))
        {
            results |= VisualGroup::CheckboxHit;
        }
    }
    else if (y < body_end)
    {
        results |= VisualGroup::BodyHit;
    }
    return results;
}

void VisualGroup::drawHeader(QPainter *painter, const QStyleOptionViewItem &option)
{
    QRect optRect = option.rect;
    optRect.setTop(optRect.top() + 7);
    QFont font(QApplication::font());
    font.setBold(true);
    const QFontMetrics fontMetrics = QFontMetrics(font);

    int centerHeight = optRect.top() + fontMetrics.height()/2;

    QPen pen;
    pen.setWidth(2);
    QColor penColor = option.palette.text().color();
    penColor.setAlphaF(0.6);
    pen.setColor(penColor);
    painter->setPen(pen);

    int arrowOffsetLeft = fontMetrics.height()/2 + 7;
    int textOffsetLeft = arrowOffsetLeft *2;
    int arrowSize = 6;

    //BEGIN: arrow
    {
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->save();

        QPolygon polygon;
        if (collapsed) {
            polygon << QPoint(arrowOffsetLeft - arrowSize/2, centerHeight - arrowSize) << QPoint(arrowOffsetLeft + arrowSize/2, centerHeight) << QPoint(arrowOffsetLeft - arrowSize/2, centerHeight + arrowSize);
            painter->drawPolyline(polygon);
        } else {
            polygon << QPoint(arrowOffsetLeft - arrowSize, centerHeight - arrowSize/2) << QPoint(arrowOffsetLeft, centerHeight + arrowSize/2) << QPoint(arrowOffsetLeft + arrowSize, centerHeight - arrowSize/2);
            painter->drawPolyline(polygon);
        }
        painter->restore();
    }
    //END: arrow

    //BEGIN: text
    {
        painter->setRenderHint(QPainter::Antialiasing);
        QRect textRect(optRect);
        textRect.setTop(textRect.top());
        textRect.setLeft(textOffsetLeft);
        textRect.setHeight(fontMetrics.height());
        textRect.setRight(textRect.right() - 7);

        painter->save();
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text != "" ? text : QObject::tr("Ungrouped"));
        painter->restore();
    }
    //END: text
}

int VisualGroup::totalHeight() const
{
    return headerHeight() + contentHeight();
}

int VisualGroup::headerHeight() const
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

int VisualGroup::contentHeight() const
{
    if (collapsed)
    {
        return 0;
    }
    auto last = rows[numRows() - 1];
    return last.top + last.height;
}

int VisualGroup::numRows() const
{
    return rows.size();
}

int VisualGroup::verticalPosition() const
{
    return m_verticalPosition;
}

QList<QModelIndex> VisualGroup::items() const
{
    QList<QModelIndex> indices;
    for (int i = 0; i < view->model()->rowCount(); ++i)
    {
        const QModelIndex index = view->model()->index(i, 0);
        if (index.data(InstanceViewRoles::GroupRole).toString() == text)
        {
            indices.append(index);
        }
    }
    return indices;
}
