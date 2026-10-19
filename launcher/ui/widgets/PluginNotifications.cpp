// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginNotifications.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int s_margin = 12;
constexpr int s_spacing = 8;
constexpr int s_width = 340;

QIcon levelIcon(PrismApi::NotifyLevel level)
{
    switch (level) {
        case PrismApi::NotifyLevel::Warning:
            return QIcon::fromTheme("status-yellow");
        case PrismApi::NotifyLevel::Error:
            return QIcon::fromTheme("status-bad");
        case PrismApi::NotifyLevel::Info:
            break;
    }
    return QIcon::fromTheme("plugins");
}

}  // namespace

PluginNotifications::PluginNotifications(QWidget* window) : QObject(window), m_window(window)
{
    m_window->installEventFilter(this);
}

void PluginNotifications::show(const QString& title, const QString& text, PrismApi::NotifyLevel level)
{
    auto* popup = new QFrame(m_window);
    popup->setFrameShape(QFrame::StyledPanel);
    popup->setAutoFillBackground(true);
    popup->setBackgroundRole(QPalette::ToolTipBase);
    popup->setForegroundRole(QPalette::ToolTipText);
    popup->setFixedWidth(s_width);

    auto* icon = new QLabel(popup);
    icon->setPixmap(levelIcon(level).pixmap(24, 24));
    icon->setAlignment(Qt::AlignTop);

    auto* titleLabel = new QLabel(popup);
    titleLabel->setTextFormat(Qt::PlainText);
    titleLabel->setText(title);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    // Plain text, so plugins cannot inject links or markup
    auto* textLabel = new QLabel(popup);
    textLabel->setTextFormat(Qt::PlainText);
    textLabel->setText(text);
    textLabel->setWordWrap(true);
    textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* close = new QToolButton(popup);
    close->setAutoRaise(true);
    close->setText(QStringLiteral("✕"));
    close->setToolTip(tr("Close"));
    connect(close, &QToolButton::clicked, popup, &QObject::deleteLater);

    auto* texts = new QVBoxLayout;
    texts->addWidget(titleLabel);
    texts->addWidget(textLabel);
    auto* layout = new QHBoxLayout(popup);
    layout->addWidget(icon);
    layout->addLayout(texts, 1);
    layout->addWidget(close, 0, Qt::AlignTop);

    QTimer::singleShot(s_timeoutMs, popup, &QObject::deleteLater);
    // Laid out once it is gone, when its pointer is already null
    connect(popup, &QObject::destroyed, this, [this] { QTimer::singleShot(0, this, &PluginNotifications::layoutPopups); });

    m_popups.append(popup);
    m_popups.removeIf([](const QPointer<QFrame>& existing) { return existing.isNull(); });
    while (m_popups.size() > s_maxVisible) {
        m_popups.takeFirst()->deleteLater();
    }

    popup->adjustSize();
    popup->show();
    popup->raise();
    layoutPopups();
}

bool PluginNotifications::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && event->type() == QEvent::Resize) {
        layoutPopups();
    }
    return QObject::eventFilter(watched, event);
}

void PluginNotifications::layoutPopups()
{
    m_popups.removeIf([](const QPointer<QFrame>& popup) { return popup.isNull(); });

    int bottom = m_window->height() - s_margin;
    if (auto* mainWindow = qobject_cast<QMainWindow*>(m_window); mainWindow && mainWindow->statusBar()->isVisible()) {
        bottom -= mainWindow->statusBar()->height();
    }
    // Newest at the bottom
    for (auto it = m_popups.crbegin(); it != m_popups.crend(); ++it) {
        QFrame* popup = *it;
        popup->adjustSize();
        bottom -= popup->height();
        popup->move(m_window->width() - popup->width() - s_margin, bottom);
        popup->raise();
        bottom -= s_spacing;
    }
}
