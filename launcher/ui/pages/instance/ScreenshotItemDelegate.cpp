// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLaucher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "ScreenshotItemDelegate.h"
#include "ui/widgets/WidgetDelegate.h"

#include <QApplication>

// ----------------
// Screenshot Card
// ----------------

// #region ScreenshotCard

ScreenshotCard::ScreenshotCard(QWidget* parent) : QFrame(parent)
{
    setFocusPolicy(Qt::NoFocus);

    // don't "leave" when mouse over a child, must process mouse events in the card
    m_screenshot.setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label.setAlignment(Qt::AlignHCenter);

    m_label.setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label.setMaximumWidth(192);
    m_label.setWordWrap(true);

    m_layout.addWidget(&m_screenshot, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    m_layout.addWidget(&m_label, 0, Qt::AlignHCenter);
    setLayout(&m_layout);
}

void ScreenshotCard::setScreenshot(const QPixmap& pixmap, const QString& name)
{
    m_screenshot.setPixmap(pixmap);
    m_label.setText(name);
    m_hovered = false;

    update();
}

void ScreenshotCard::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;

    const QColor color = palette().color(selected ? QPalette::HighlightedText : QPalette::WindowText);
    QPalette label = m_label.palette();
    label.setColor(QPalette::WindowText, color);
    label.setColor(QPalette::Text, color);
    m_label.setPalette(label);

    update();
}

void ScreenshotCard::setCurrent(bool current)
{
    if (m_current == current) {
        return;
    }
    m_current = current;

    update();
}

void ScreenshotCard::enterEvent(QEnterEvent* /*event*/)
{
    setHovered(true);
}

void ScreenshotCard::leaveEvent(QEvent* /*event*/)
{
    setHovered(false);
}

void ScreenshotCard::setHovered(bool hovered)
{
    if (m_hovered == hovered) {
        return;
    }
    m_hovered = hovered;

    update();
}

void ScreenshotCard::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QStyleOptionViewItem option;
    option.initFrom(this);  // palette, font, direction, rect, State_Enabled/Active
    option.widget = this;   // for stylesheet-accurate resolution
    option.rect = rect();
    option.showDecorationSelected = true;  // make sure highlight works

    option.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus);
    if (m_selected) {
        option.state |= QStyle::State_Selected;
    }
    if (m_hovered) {
        option.state |= QStyle::State_MouseOver;
    }
    if (m_current) {
        option.state |= QStyle::State_HasFocus;
    }

    style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, &painter, this);
}

// #endregion

// --------------------
// Screenshot Delegate
// --------------------

// #region ScreenshotItemDelegate

ScreenshotItemDelegate::ScreenshotItemDelegate(QObject* parent)
    : WidgetDelegate([](QWidget* parent) -> QWidget* { return new ScreenshotCard(parent); }, parent)
{}

QSize ScreenshotItemDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    return ScreenshotCard::CardSize;
}

void ScreenshotItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (auto* card = qobject_cast<ScreenshotCard*>(editor)) {
        const auto screenshot = index.data(Qt::DecorationRole).value<QIcon>().pixmap({ 128, 128 });
        const auto name = index.data(Qt::DisplayRole).toString();  // get name

        card->setScreenshot(screenshot, name);
    }
}

void ScreenshotItemDelegate::paintItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // draw background, for selected and non selected
    QStyle* style = option.widget ? option.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);
}

void ScreenshotItemDelegate::updateEditorState(QWidget* editor, const QModelIndex& index) const
{
    if (auto* card = qobject_cast<ScreenshotCard*>(editor)) {
        if (auto* view = this->view()) {
            card->setSelected(view->selectionModel()->isSelected(index));
            card->setCurrent(view->currentIndex() == index);
        }
    }
}

// #endregion
