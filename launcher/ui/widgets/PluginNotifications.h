// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QObject>
#include <QPointer>

#include "plugins/lua/PrismApi.h"

class QFrame;
class QWidget;

// Shows plugin notifications as small popups stacked at the bottom right of a window. They close by themselves after a
// while, and only the newest few stay visible.
class PluginNotifications : public QObject {
    Q_OBJECT

   public:
    static constexpr int s_maxVisible = 3;
    static constexpr int s_timeoutMs = 8000;

    explicit PluginNotifications(QWidget* window);

    void show(const QString& title, const QString& text, PrismApi::NotifyLevel level);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

   private:
    void layoutPopups();

    QWidget* m_window;
    // Oldest first
    QList<QPointer<QFrame>> m_popups;
};
