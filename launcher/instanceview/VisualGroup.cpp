/* Copyright 2013-2021 MultiMC Contributors
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
        auto itemHeight = view->itemDelegate()->sizeHint(view->viewOptions(), item).height();
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

int VisualGroup::totalHeight() const
{
    return headerHeight() + 5 + contentHeight(); // FIXME: wtf is that '5'?
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
