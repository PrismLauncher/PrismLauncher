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

#include "ui/GuiUtil.h"
#include "ui/widgets/WidgetDelegate.h"

#include <QApplication>

#include <QCursor>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStackedLayout>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// ----------------

namespace {

class ActionPanel : public QWidget {
   public:
    using QWidget::QWidget;

   protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QColor bg = palette().color(QPalette::Window);
        // lighter themes need more contrast to stay readable
        bg.setAlpha(bg.lightness() > 127 ? 160 : 120);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bg);
        painter.drawRoundedRect(QRectF(rect()), 4, 4);
    }
};

}  // namespace

// ----------------
// Screenshot Card
// ----------------

ScreenshotCard::ScreenshotCard(QWidget* parent)
    : QFrame(parent)
    , m_screenshot(new QLabel(this))
    , m_label(new QLabel(this))
    , m_edit(new QLineEdit(this))
    , m_labelStack(new QStackedLayout)
    , m_actions(new ActionPanel(this))
    , m_rename(new QToolButton(this))
    , m_delete(new QToolButton(this))
    , m_copy(new QToolButton(this))
{
    setFocusPolicy(Qt::NoFocus);

    // set up margins and sizes
    m_layout.setContentsMargins(CardMargin, CardMargin, CardMargin, CardMargin);
    m_layout.setSpacing(4);

    m_screenshot->setAlignment(Qt::AlignCenter);

    // don't "leave" when mouse over a child, must process mouse events in the card
    m_screenshot->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    // align and wrap label
    m_label->setAlignment(Qt::AlignHCenter);
    m_label->setWordWrap(true);

    // align and frame edit
    m_edit->setAlignment(Qt::AlignHCenter);
    m_edit->installEventFilter(this);  // for escape and return processing
    m_edit->setFrame(true);
    connect(m_edit, &QLineEdit::editingFinished, this, [this] { endRename(true); });

    // setup label edit stack
    m_labelStack->addWidget(m_label);
    m_labelStack->addWidget(m_edit);
    m_labelStack->setStackingMode(QStackedLayout::StackOne);

    // add to layout
    m_screenshot->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_layout.addWidget(m_screenshot, 1);
    m_layout.addLayout(m_labelStack, 0);

    // setup action buttons
    m_rename->setIcon(QIcon::fromTheme("rename"));
    m_rename->setIconSize({ 16, 16 });
    m_rename->setToolTip(tr("Rename"));
    m_rename->setFocusPolicy(Qt::NoFocus);
    m_rename->setAutoRaise(true);

    m_delete->setIcon(QIcon::fromTheme("delete"));
    m_delete->setIconSize({ 16, 16 });
    m_delete->setToolTip(tr("Delete"));
    m_delete->setFocusPolicy(Qt::NoFocus);
    m_delete->setAutoRaise(true);

    m_copy->setIcon(QIcon::fromTheme("copy"));
    m_copy->setIconSize({ 16, 16 });
    m_copy->setToolTip(tr("Copy Image"));
    m_copy->setFocusPolicy(Qt::NoFocus);
    m_copy->setAutoRaise(true);

    auto* actionsLayout = new QHBoxLayout(m_actions);
    actionsLayout->setContentsMargins(2, 2, 2, 2);
    actionsLayout->setSpacing(0);
    actionsLayout->addWidget(m_copy);
    actionsLayout->addWidget(m_rename);
    actionsLayout->addWidget(m_delete);

    m_actions->hide();

    // the animation targets the effect, so the effect must be installed first
    // and can't be initialized independently in ctor list
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    m_actionsOpacity = new QGraphicsOpacityEffect;
    m_actionsOpacity->setOpacity(0.0);
    m_actions->setGraphicsEffect(m_actionsOpacity);  // m_actions takes ownership of the effect

    m_actionsFade = new QPropertyAnimation(m_actionsOpacity, "opacity", this);
    m_actionsFade->setDuration(120);
    m_actionsFade->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_actionsFade, &QPropertyAnimation::finished, this, [this] {
        if (!m_hovered) {
            m_actions->hide();
        }
    });

    setLayout(&m_layout);

    connect(m_rename, &QToolButton::clicked, this, [this](bool /*checked*/) { startRename(); });
    connect(m_delete, &QToolButton::clicked, this, [this](bool /*checked*/) {
        if (m_index.isValid()) {
            emit deleteRequested(m_index);
        }
    });
    connect(m_copy, &QToolButton::clicked, this, [this](bool /*checked*/) {
        if (m_index.isValid()) {
            emit copyRequested(m_index);
        }
    });
}

void ScreenshotCard::setScreenshot(const QPixmap& pixmap, const QString& name)
{
    m_thumbnail = pixmap;
    updateThumbnail();

    m_label->setText(name);

    m_actionsFade->stop();
    m_hovered = false;
    m_actionsOpacity->setOpacity(0.0);
    m_actions->hide();

    endRename(false);

    update();
}

QSize ScreenshotCard::thumbnailArea() const
{
    const int w = width() - (2 * CardMargin);
    const int h = height() - (2 * CardMargin) - m_label->sizeHint().height() - m_layout.spacing();
    return { qMax(0, w), qMax(0, h) };
}

void ScreenshotCard::updateThumbnail()
{
    if (!m_thumbnail.isNull()) {
        m_screenshot->setPixmap(m_thumbnail.scaled(thumbnailArea(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ScreenshotCard::updateLabelPalette()
{
    QPalette p = palette();

    const QColor color = m_selected ? GuiUtil::selectedItemFg(p, style()) : p.color(QPalette::WindowText);

    p.setColor(QPalette::WindowText, color);
    p.setColor(QPalette::Text, color);
    m_label->setPalette(p);
}

void ScreenshotCard::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;

    updateLabelPalette();

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

// scrolling doesn't deliver enter / leave events so the
// real position test is needed instead of `underMouse()`
bool ScreenshotCard::cursorInside() const
{
    return rect().contains(mapFromGlobal(QCursor::pos()));
}

void ScreenshotCard::enterEvent(QEnterEvent* /*event*/)
{
    setHovered(true);
}

void ScreenshotCard::leaveEvent(QEvent* /*event*/)
{
    setHovered(cursorInside());
}

void ScreenshotCard::setHovered(bool hovered)
{
    if (m_hovered == hovered) {
        return;
    }
    m_hovered = hovered;

    m_actionsFade->stop();  // retarget mid-fade
    m_actionsFade->setStartValue(m_actionsOpacity->opacity());
    m_actionsFade->setEndValue(hovered ? 1.0 : 0.0);
    if (hovered) {
        m_actions->show();  // animation finish will hide
    }
    m_actionsFade->start();
    // prevent clicks during fade
    m_actions->setAttribute(Qt::WA_TransparentForMouseEvents, !hovered);
    update();
}

void ScreenshotCard::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QStyleOptionViewItem option;
    const QWidget* w = m_view ? static_cast<QWidget*>(m_view) : this;
    option.initFrom(w);
    option.widget = w;
    option.rect = rect();
    option.showDecorationSelected = true;

    // clear these, can't have them under the real widget
    option.text.clear();
    option.icon = QIcon();

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

    // I resisted using drawControl over drawPrimitive because of the need to clear the
    // text and icon it would draw from its `option` but it appears to be the only real way
    // to get a correct style across themes.
    style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, option.widget);
}

void ScreenshotCard::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    const QSize hint = m_actions->sizeHint();
    m_actions->setGeometry(width() - hint.width() - ActionInset, ActionInset, hint.width(), hint.height());
    updateThumbnail();
}

void ScreenshotCard::changeEvent(QEvent* event)
{
    QFrame::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        updateLabelPalette();
    }
}

void ScreenshotCard::moveEvent(QMoveEvent* event)
{
    QFrame::moveEvent(event);
    // a scroll can move the card out from under a stationary pointer which
    // produces no enter/leave events
    setHovered(cursorInside());
}

void ScreenshotCard::mousePressEvent(QMouseEvent* event)
{
    if (m_selected && event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier &&
        m_label->geometry().contains(event->position().toPoint())) {
        startRename();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void ScreenshotCard::setIndex(const QModelIndex& index)
{
    m_index = index;
    updateActionButtons();
}

void ScreenshotCard::updateActionButtons()
{
    const bool writable = m_index.isValid() && ((m_index.flags() & Qt::ItemIsEditable) != 0);
    m_rename->setEnabled(writable);
    m_delete->setEnabled(writable);

    // copying only reads the file
    m_copy->setEnabled(m_index.isValid());
}

void ScreenshotCard::startRename()
{
    if (m_editing || !m_index.isValid() || !(m_index.flags() & Qt::ItemIsEditable)) {
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

// --------------------
// Screenshot Delegate
// --------------------

ScreenshotItemDelegate::ScreenshotItemDelegate(QObject* parent)
    : WidgetDelegate(
          [this](QWidget* parent) -> QWidget* {
              auto* card = new ScreenshotCard(parent);
              connect(card, &ScreenshotCard::renameRequested, this, &ScreenshotItemDelegate::renameRequested);
              connect(card, &ScreenshotCard::deleteRequested, this, &ScreenshotItemDelegate::deleteRequested);
              connect(card, &ScreenshotCard::copyRequested, this, &ScreenshotItemDelegate::copyRequested);
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
        // get thumbnail at cache size, scale in card
        const auto screenshot = index.data(Qt::DecorationRole).value<QIcon>().pixmap({ 256, 256 });
        const auto name = index.data(Qt::DisplayRole).toString();  // get name

        card->setScreenshot(screenshot, name);
        card->setIndex(index);
        card->setView(view());
    }
}

void ScreenshotItemDelegate::paintItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    // draw background, for selected and non selected
    const QStyle* style = option.widget ? option.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);
}

void ScreenshotItemDelegate::updateEditorState(QWidget* editor, const QModelIndex& index) const
{
    auto* v = view();
    if (!v) {
        return;
    }

    auto* card = qobject_cast<ScreenshotCard*>(editor);
    if (!card) {
        return;
    }

    // needed so real size after full layout populates
    // otherwise the editor fits to contents
    const QRect rect = v->visualRect(index);
    if (rect.isValid() && card->geometry() != rect) {
        card->setGeometry(rect);  // triggers resizeEvent -> rescale
    }
    card->setSelected(v->selectionModel()->isSelected(index));
    card->setCurrent(v->currentIndex() == index);
}

void ScreenshotItemDelegate::startRename(const QModelIndex& index)
{
    auto* v = view();
    if (!v) {
        return;
    }

    auto* card = qobject_cast<ScreenshotCard*>(v->indexWidget(index));
    if (!card) {
        return;
    }
    card->startRename();
}
