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

#pragma once

#include "ui/widgets/WidgetDelegate.h"

#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

class ScreenshotCard : public QFrame {
    Q_OBJECT

   public:
    explicit ScreenshotCard(QWidget* parent = nullptr);

    static constexpr QSize CardSize = { 192, 160 };

   public:
    void setScreenshot(const QPixmap&, const QString& name);
    void setSelected(bool selected);
    void setHovered(bool hovered);
    void setCurrent(bool current);

   protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

   private:
    QLabel m_screenshot, m_label;

    QVBoxLayout m_layout;

    bool m_selected, m_hovered, m_current = false;
};

class ScreenshotItemDelegate : public WidgetDelegate {
    Q_OBJECT

   public:
    explicit ScreenshotItemDelegate(QObject* parent);

   public:
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void paintItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void updateEditorState(QWidget* editor, const QModelIndex& index) const override;
};
