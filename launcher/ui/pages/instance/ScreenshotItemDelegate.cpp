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

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>

// ----------------
// Screenshot Card
// ----------------

// #region ScreenshotCard

ScreenshotCard::ScreenshotCard(QWidget* parent)
    : QFrame(parent)
    , m_screenshot(new QLabel(this))
    , m_label(new QLabel(this))
    , m_edit(new QLineEdit(this))
    , m_labelStack(new QStackedLayout)
    , m_layout(new QVBoxLayout)
{
    setFocusPolicy(Qt::NoFocus);

    // don't "leave" when mouse over a child, must process mouse events in the card
    m_screenshot->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label->setAlignment(Qt::AlignHCenter);

    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label->setWordWrap(true);

    m_edit->setAlignment(Qt::AlignHCenter);
    m_edit->installEventFilter(this);  // for escape and return processing
    m_edit->setFrame(true);
    connect(m_edit, &QLineEdit::editingFinished, this, [this] { endRename(true); });

    m_labelStack->addWidget(m_label);
    m_labelStack->addWidget(m_edit);
    m_labelStack->setStackingMode(QStackedLayout::StackOne);

    m_layout->addWidget(m_screenshot, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    m_layout->addLayout(m_labelStack);

    setLayout(m_layout);
}

void ScreenshotCard::setScreenshot(const QPixmap& pixmap, const QString& name)
{
    m_screenshot->setPixmap(pixmap);
    m_label->setText(name);
    m_hovered = false;

    endRename(false);

    update();
}

void ScreenshotCard::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;

    const QColor color = palette().color(selected ? QPalette::HighlightedText : QPalette::WindowText);
    QPalette label = m_label->palette();
    label.setColor(QPalette::WindowText, color);
    label.setColor(QPalette::Text, color);
    m_label->setPalette(label);

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

void ScreenshotCard::setIndex(const QModelIndex& index)
{
    m_index = index;
}

void ScreenshotCard::startRename()
{
    if (m_editing || !m_index.isValid()) {
        return;
    }

    m_editing = true;

    m_edit->setText(m_label->text());
    m_labelStack->setCurrentWidget(m_edit);

    // defer the focus to the event loop after this action run so focus isn't clobbered
    QTimer::singleShot(0, m_edit, [this] {
        m_edit->setFocus(Qt::OtherFocusReason);
        m_edit->selectAll();
    });
}

void ScreenshotCard::endRename(bool commit)
{
    if (!m_editing) {
        return;
    }
    m_editing = false;

    const QString name = m_edit->text().trimmed();

    m_labelStack->setCurrentWidget(m_label);

    if (commit && !name.isEmpty() && name != m_label->text()) {
        emit renameRequested(m_index, name);
    }
}

bool ScreenshotCard::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_edit && event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Escape) {
            endRename(false);
            return true;
        }
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            endRename(true);
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

// #endregion

// --------------------
// Screenshot Delegate
// --------------------

// #region ScreenshotItemDelegate

ScreenshotItemDelegate::ScreenshotItemDelegate(QObject* parent)
    : WidgetDelegate(
          [this](QWidget* parent) -> QWidget* {
              auto* card = new ScreenshotCard(parent);
              connect(card, &ScreenshotCard::renameRequested, this, &ScreenshotItemDelegate::renameRequested);
              return card;
          },
          parent)
{}

void ScreenshotItemDelegate::destroyEditor(QWidget* editor, const QModelIndex& index) const
{
    if (auto* card = qobject_cast<ScreenshotCard*>(editor)) {
        card->cancelRename();
    }
    WidgetDelegate::destroyEditor(editor, index);
}

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
        card->setIndex(index);
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

void ScreenshotItemDelegate::startRename(const QModelIndex& index)
{
    if (auto* v = view()) {
        if (auto* editor = v->indexWidget(index)) {
            if (auto* card = qobject_cast<ScreenshotCard*>(editor)) {
                card->startRename();
            }
        }
    }
}

// #endregion
