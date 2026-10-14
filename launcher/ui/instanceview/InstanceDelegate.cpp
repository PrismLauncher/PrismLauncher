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
#include <QTextEdit>
#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "ui/themes/ThemeManager.h"

namespace {
struct SlotLayout {
    QRect slot;
    QRect text;
    QFont font;
    int lineHeight = 0;
    // Optional line under the name: Minecraft version and mod loader
    QRect meta;
    QFont metaFont;
};

SlotLayout slotLayout(const QRect& rect, const ThemeTokens& tokens, const QFont& fallbackFont)
{
    SlotLayout layout;
    const int unit = tokens.pixelUnit;
    // Room for the icon plus a four-pixel border, as in an inventory slot
    const int slotSize = tokens.tileIconSize + 8 * unit;
    // The selection frame sits two pixels outside the slot, so leave that much space above it
    layout.slot = QRect(rect.left() + (rect.width() - slotSize) / 2, rect.top() + 2 * unit, slotSize, slotSize);
    layout.font = fallbackFont;
    if (!tokens.displayFont.isEmpty()) {
        layout.font = QFont(tokens.displayFont);
        if (tokens.displayFontPixelSize > 0) {
            layout.font.setPixelSize(tokens.displayFontPixelSize);
        }
    }
    layout.lineHeight = QFontMetrics(layout.font).height();
    layout.text = QRect(rect.left() + unit, layout.slot.bottom() + 1 + 4 * unit, rect.width() - 2 * unit, 2 * layout.lineHeight);
    if (tokens.tileMetaLine) {
        layout.metaFont = QApplication::font();
        layout.metaFont.setPixelSize(qMax(10, tokens.displayFontPixelSize - 4));
        layout.meta = QRect(layout.text.left(), layout.text.bottom() + 1, layout.text.width(), QFontMetrics(layout.metaFont).height());
    }
    return layout;
}

int slotTileHeight(const SlotLayout& layout, const ThemeTokens& tokens)
{
    const QRect& last = layout.meta.isValid() ? layout.meta : layout.text;
    return last.bottom() + 1 + 2 * tokens.pixelUnit;
}

QString packSourceIcon(const QString& source)
{
    return source == "ftb" ? QStringLiteral("ftb_logo") : source;
}

// Wraps text onto at most two lines and elides whatever doesn't fit on the second
QStringList twoLines(const QString& text, const QFont& font, int width)
{
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);
    layout.beginLayout();
    QStringList lines;
    QTextLine line = layout.createLine();
    if (line.isValid()) {
        line.setLineWidth(width);
        lines << text.mid(line.textStart(), line.textLength()).trimmed();
        const QString rest = text.mid(line.textStart() + line.textLength()).trimmed();
        if (!rest.isEmpty()) {
            lines << QFontMetrics(font).elidedText(rest, Qt::ElideRight, width);
        }
    }
    layout.endLayout();
    return lines;
}

void fillFrame(QPainter* painter, const QRect& rect, int width, const QColor& color)
{
    painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), width), color);
    painter->fillRect(QRect(rect.left(), rect.bottom() - width + 1, rect.width(), width), color);
    painter->fillRect(QRect(rect.left(), rect.top() + width, width, rect.height() - 2 * width), color);
    painter->fillRect(QRect(rect.right() - width + 1, rect.top() + width, width, rect.height() - 2 * width), color);
}
}  // namespace

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

void drawSelectionRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    if ((option.state & QStyle::State_Selected))
        painter->fillRect(rect, option.palette.brush(QPalette::Highlight));
    else {
        QColor backgroundColor = option.palette.color(QPalette::Window);
        backgroundColor.setAlpha(160);
        painter->fillRect(rect, QBrush(backgroundColor));
    }
}

void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state)
{
    QList<QString> pixmaps;
    if (instance->isRunning()) {
        pixmaps.append("status-running");
    } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
        pixmaps.append("status-bad");
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

void ListViewDelegate::paintSlot(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index,
                                 const ThemeTokens& tokens) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);

    const auto layout = slotLayout(opt.rect, tokens, opt.font);
    const QRect& slot = layout.slot;
    const int unit = tokens.pixelUnit;
    const QIcon::Mode mode = (opt.state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled;

    // A recessed square: dark top-left edge, light bottom-right edge
    painter->fillRect(slot, tokens.slotFill);
    painter->fillRect(QRect(slot.left(), slot.top(), slot.width(), unit), tokens.slotShadow);
    painter->fillRect(QRect(slot.left(), slot.top(), unit, slot.height()), tokens.slotShadow);
    painter->fillRect(QRect(slot.left() + unit, slot.bottom() - unit + 1, slot.width() - unit, unit), tokens.slotHighlight);
    painter->fillRect(QRect(slot.right() - unit + 1, slot.top() + unit, unit, slot.height() - unit), tokens.slotHighlight);

    opt.icon.paint(painter, slot.adjusted(4 * unit, 4 * unit, -4 * unit, -4 * unit), Qt::AlignCenter, mode, QIcon::Off);

    // Like an inventory, the slot under the pointer is washed with white
    if ((opt.state & QStyle::State_MouseOver) && tokens.slotHover.isValid()) {
        painter->fillRect(slot.adjusted(unit, unit, -unit, -unit), tokens.slotHover);
    }
    // The selected slot gets a frame, like the active hotbar slot
    if (opt.state & QStyle::State_Selected) {
        const QColor frame = tokens.selectionFrame.isValid() ? tokens.selectionFrame : opt.palette.color(QPalette::Highlight);
        fillFrame(painter, slot.adjusted(-2 * unit, -2 * unit, 2 * unit, 2 * unit), unit, frame);
    }

    // FIXME: this really has no business of being here. Make generic.
    if (auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole).value<void*>()) {
        QString badge;
        if (instance->isRunning()) {
            badge = "status-running";
        } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
            badge = "status-bad";
        }
        if (!badge.isEmpty()) {
            const QRect badgeRect(slot.right() - unit - 16, slot.top() + unit + 1, 16, 16);
            QIcon::fromTheme(badge).paint(painter, badgeRect, Qt::AlignCenter, mode, QIcon::Off);
        }
    }
    if (tokens.tileMetaLine) {
        // Where the modpack came from, tucked into the slot's corner like an item's enchantment glint
        if (const QString source = index.data(InstanceList::PackSourceRole).toString(); !source.isEmpty()) {
            const QRect sourceRect(slot.right() - unit - 16, slot.bottom() - unit - 16, 16, 16);
            QIcon::fromTheme(packSourceIcon(source)).paint(painter, sourceRect, Qt::AlignCenter, mode, QIcon::Off);
        }
    }

    // A translucent box hugging the text, like a name tag
    const auto drawBacking = [&](const QRect& lineRect, const QFontMetrics& metrics, const QString& text) {
        if (!tokens.viewTextBacking.isValid() || text.isEmpty()) {
            return;
        }
        const int width = qMin(lineRect.width(), metrics.horizontalAdvance(text) + 2 * unit);
        painter->fillRect(QRect(lineRect.center().x() - width / 2, lineRect.top(), width, lineRect.height()), tokens.viewTextBacking);
    };

    painter->setFont(layout.font);
    const QColor textColor = tokens.viewText.isValid() ? tokens.viewText : opt.palette.color(QPalette::Text);
    const QFontMetrics nameMetrics(layout.font);
    int top = layout.text.top();
    for (const auto& line : twoLines(opt.text, layout.font, layout.text.width())) {
        const QRect lineRect(layout.text.left(), top, layout.text.width(), layout.lineHeight);
        drawBacking(lineRect, nameMetrics, line);
        if (tokens.viewTextShadow.isValid()) {
            painter->setPen(tokens.viewTextShadow);
            painter->drawText(lineRect.translated(1, 1), Qt::AlignHCenter | Qt::AlignTop, line);
        }
        painter->setPen(textColor);
        painter->drawText(lineRect, Qt::AlignHCenter | Qt::AlignTop, line);
        top += layout.lineHeight;
    }

    if (layout.meta.isValid()) {
        QString meta = index.data(InstanceList::MinecraftVersionRole).toString();
        if (const QString loader = index.data(InstanceList::ModLoaderRole).toString(); !loader.isEmpty()) {
            meta = meta.isEmpty() ? loader : QStringLiteral("%1 · %2").arg(meta, loader);
        }
        if (!meta.isEmpty()) {
            painter->setFont(layout.metaFont);
            const QFontMetrics metaMetrics(layout.metaFont);
            meta = metaMetrics.elidedText(meta, Qt::ElideRight, layout.meta.width());
            drawBacking(layout.meta, metaMetrics, meta);
            if (tokens.viewTextShadow.isValid()) {
                painter->setPen(tokens.viewTextShadow);
                painter->drawText(layout.meta.translated(1, 1), Qt::AlignHCenter | Qt::AlignTop, meta);
            }
            QColor metaColor = textColor;
            metaColor.setAlpha(200);
            painter->setPen(metaColor);
            painter->drawText(layout.meta, Qt::AlignHCenter | Qt::AlignTop, meta);
        }
    }

    painter->restore();
}

void ListViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (const auto& tokens = APPLICATION->themeManager()->getTokens(); tokens.tileStyle == ThemeTokens::TileStyle::Slot) {
        paintSlot(painter, option, index, tokens);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);

    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    const int iconSize = 48;
    QRect iconbox = opt.rect;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) + 1;
    QRect textRect = opt.rect;
    QRect textHighlightRect = textRect;
    // clip the decoration on top, remove width padding
    textRect.adjust(textMargin, iconSize + textMargin + 5, -textMargin, 0);

    textHighlightRect.adjust(0, iconSize + 5, 0, 0);

    // draw background
    drawSelectionRect(painter, opt, textHighlightRect);

    // icon mode and state, also used for badges
    QIcon::Mode mode = QIcon::Normal;
    if (!(opt.state & QStyle::State_Enabled))
        mode = QIcon::Disabled;
    else if (opt.state & QStyle::State_Selected)
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
    if (opt.state & QStyle::State_Selected) {
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

    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (const auto& tokens = APPLICATION->themeManager()->getTokens(); tokens.tileStyle == ThemeTokens::TileStyle::Slot) {
        // Every slot tile has room for two lines of text, so rows line up like an inventory grid
        const auto layout = slotLayout(QRect(0, 0, tokens.tileWidth, 0), tokens, option.font);
        return QSize(tokens.tileWidth, slotTileHeight(layout, tokens));
    }

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
    if (const auto& tokens = APPLICATION->themeManager()->getTokens(); tokens.tileStyle == ThemeTokens::TileStyle::Slot) {
        editor->setGeometry(slotLayout(option.rect, tokens, option.font).text);
        return;
    }

    const int iconSize = 48;
    QRect textRect = option.rect;
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
