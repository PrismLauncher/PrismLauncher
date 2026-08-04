// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "InstanceDelegate.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QTextLayout>
#include <QTextOption>
#include <QtMath>

#include <QIcon>
#include <QStyle>
#include <QTextEdit>
#include <QWidget>
#include "BaseInstance.h"
#include "InstanceList.h"
#include "InstanceView.h"

// Origin: Qt
static void viewItemTextLayout(QTextLayout& textLayout, int lineWidth, qreal& height, qreal& widthUsed)
{
    height = 0;
    widthUsed = 0;
    textLayout.beginLayout();
    QString str = textLayout.text();
    while (true) {
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

ListViewDelegate::ListViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

static bool highlightSelectedText(const QStyleOptionViewItem& option)
{
    if (option.widget) {
        const QVariant value = option.widget->property("highlightSelectedText");
        if (value.isValid())
            return value.toBool();
    }
    return false;
}

static bool highlightSelectedIcon(const QStyleOptionViewItem& option)
{
    if (option.widget) {
        const QVariant value = option.widget->property("highlightSelectedIcon");
        if (value.isValid())
            return value.toBool();
    }
    return false;
}

static QColor mixColor(const QColor& from, const QColor& to, qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount, from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
}

// Built-in look for the instance grid, used when no theme stylesheet is loaded.
//
// Hover and selection are separated by kind rather than by strength. Hover is
// transient, so it takes the brighter fill. Selection is permanent, so it stays
// dim and is marked with a border instead - that keeps a constant block of
// colour off the instance icons, and the two states remain distinct when they
// overlap. Colours are mixed from the palette so this follows any theme,
// including light ones, instead of hardcoding greys.
static void drawDefaultItemPanel(QPainter* painter, const QStyleOptionViewItem& option)
{
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    if (!selected && !hovered)
        return;

    const QColor base = option.palette.color(QPalette::Window);
    const QColor toward = option.palette.color(QPalette::Text);

    // even tonal steps, roughly 1.1-1.2:1 apart from each other
    qreal fillAmount = 0.05;
    if (selected && hovered)
        fillAmount = 0.15;
    else if (hovered)
        fillAmount = 0.11;

    const qreal radius = 5.0;
    const QRectF rounded = QRectF(option.rect).adjusted(0.5, 0.5, -0.5, -0.5);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(mixColor(base, toward, fillAmount));
    painter->drawRoundedRect(rounded, radius, radius);

    if (selected) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(mixColor(base, toward, hovered ? 0.50 : 0.40), 1.0));
        painter->drawRoundedRect(rounded, radius, radius);
    }
    painter->restore();
}

// With a theme stylesheet loaded, hand the item panel to QStyleSheetStyle so
// InstanceView::item and its :hover / :selected rules take effect, the same way
// QTreeView::item already works. Otherwise draw the built-in look above.
static void drawItemPanel(QPainter* painter, const QStyleOptionViewItem& option)
{
    if (qApp && !qApp->styleSheet().isEmpty()) {
        QStyle* style = option.widget ? option.widget->style() : QApplication::style();
        QStyleOptionViewItem panel = option;
        panel.showDecorationSelected = true;
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &panel, painter, option.widget);
        return;
    }

    drawDefaultItemPanel(painter, option);
}

// Optional translucent plate behind the instance name, for themes that leave
// the item background transparent. Off unless a theme asks for it with
// InstanceView { qproperty-labelBackgroundAlpha: 160; }
static void drawLabelBackground(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    int alpha = 0;
    if (option.widget) {
        const QVariant value = option.widget->property("labelBackgroundAlpha");
        if (value.isValid())
            alpha = qBound(0, value.toInt(), 255);
    }
    if (alpha == 0)
        return;

    QColor backgroundColor = option.palette.color(QPalette::Window);
    backgroundColor.setAlpha(alpha);
    painter->fillRect(rect, QBrush(backgroundColor));
}

void drawSelectionRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    drawItemPanel(painter, option);
    drawLabelBackground(painter, option, rect);
}

void drawFocusRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
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

    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

    style->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, painter, option.widget);

    painter->setRenderHint(QPainter::Antialiasing);
}

// TODO this can be made a lot prettier
void drawProgressOverlay(QPainter* painter, const QStyleOptionViewItem& option, const int value, const int maximum)
{
    if (maximum == 0 || value == maximum) {
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

void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state)
{
    QList<QString> pixmaps;
    if (instance->isRunning()) {
        pixmaps.append("status-running");
    } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
        pixmaps.append("status-bad");
    }
    if (instance->hasUpdateAvailable()) {
        pixmaps.append("checkupdate");
    }

    static const int itemSide = 24;
    static const int spacing = 1;
    const int itemsPerRow = qMax(1, qFloor(double(option.rect.width() + spacing) / double(itemSide + spacing)));
    const int rows = qCeil((double)pixmaps.size() / (double)itemsPerRow);
    QListIterator<QString> it(pixmaps);
    painter->translate(option.rect.topLeft());
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < itemsPerRow; ++x) {
            if (!it.hasNext()) {
                return;
            }
            // FIXME: inject this.
            auto icon = QIcon::fromTheme(it.next());
            // opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
            const QPixmap pixmap;
            // itemSide
            QRect badgeRect(option.rect.width() - x * itemSide + qMax(x - 1, 0) * spacing - itemSide,
                            y * itemSide + qMax(y - 1, 0) * spacing, itemSide, itemSide);
            icon.paint(painter, badgeRect, Qt::AlignCenter, mode, state);
        }
    }
    painter->translate(-option.rect.topLeft());
}

static QSize viewItemTextSize(const QStyleOptionViewItem* option)
{
    QStyle* style = option->widget ? option->widget->style() : QApplication::style();
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, option, option->widget) + 1;
    QRect bounds(0, 0, 100 - 2 * textMargin, 600);
    qreal height = 0, widthUsed = 0;
    viewItemTextLayout(textLayout, bounds.width(), height, widthUsed);
    const QSize size(qCeil(widthUsed), qCeil(height));
    return QSize(size.width() + 2 * textMargin, size.height());
}

void ListViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);

    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

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
        drawSelectionRect(painter, opt, textHighlightRect);
        /*
        QPalette::ColorGroup cg;
        QStyleOptionViewItem opt2(opt);

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
        */
        /*
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
        */

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

    // icon mode and state, also used for badges
    QIcon::Mode mode = QIcon::Normal;
    if (!(opt.state & QStyle::State_Enabled))
        mode = QIcon::Disabled;
    else if ((opt.state & QStyle::State_Selected) && highlightSelectedIcon(opt))
        mode = QIcon::Selected;
    QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

    // draw the icon
    {
        iconbox.setHeight(iconSize);
        opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
    }
    // set the text colors
    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if ((opt.state & QStyle::State_Selected) && highlightSelectedText(opt)) {
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    } else {
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

    const QRect layoutRect = QStyle::alignedRect(opt.direction, opt.displayAlignment, QSize(textRect.width(), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        line.draw(painter, position);
    }

    // FIXME: this really has no business of being here. Make generic.
    auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole).value<void*>();
    if (instance) {
        drawBadges(painter, opt, instance, mode, state);
    }

    drawProgressOverlay(painter, opt, index.data(InstanceViewRoles::ProgressValueRole).toInt(),
                        index.data(InstanceViewRoles::ProgressMaximumRole).toInt());

    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, opt.widget) + 1;
    int height = 48 + textMargin * 2 + 5;  // TODO: turn constants into variables
    QSize szz = viewItemTextSize(&opt);
    height += szz.height();
    // FIXME: maybe the icon items could scale and keep proportions?
    QSize sz(100, height);
    return sz;
}

class NoReturnTextEdit : public QTextEdit {
    Q_OBJECT
   public:
    explicit NoReturnTextEdit(QWidget* parent) : QTextEdit(parent)
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
    bool event(QEvent* event) override
    {
        auto eventType = event->type();
        if (eventType == QEvent::KeyPress || eventType == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            auto key = keyEvent->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && eventType == QEvent::KeyPress) {
                emit editingDone();
                return true;
            }
            if (key == Qt::Key_Tab) {
                return true;
            }
        }
        return QTextEdit::event(event);
    }
   signals:
    void editingDone();
};

void ListViewDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            [[maybe_unused]] const QModelIndex& index) const
{
    const int iconSize = 48;
    QRect textRect = option.rect;
    // QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    textRect.adjust(0, iconSize + 5, 0, 0);
    editor->setGeometry(textRect);
}

void ListViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto text = index.data(Qt::EditRole).toString();
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    realEditor->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    realEditor->append(text);
    realEditor->selectAll();
    realEditor->document()->clearUndoRedoStacks();
}

void ListViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    QString text = realEditor->toPlainText();
    text.replace(QChar('\n'), QChar(' '));
    text = text.trimmed();
    // Prevent instance names longer than 128 chars
    text.truncate(128);
    if (text.size() != 0) {
        const auto before = model->data(index).toString();
        model->setData(index, text);
        emit textChanged(before, text);
    }
}

QWidget* ListViewDelegate::createEditor(QWidget* parent,
                                        [[maybe_unused]] const QStyleOptionViewItem& option,
                                        [[maybe_unused]] const QModelIndex& index) const
{
    auto editor = new NoReturnTextEdit(parent);
    connect(editor, &NoReturnTextEdit::editingDone, this, &ListViewDelegate::editingDone);
    return editor;
}

void ListViewDelegate::editingDone()
{
    NoReturnTextEdit* editor = qobject_cast<NoReturnTextEdit*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

#include "InstanceDelegate.moc"
