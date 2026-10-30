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
#include <QPixmap>
#include <QVBoxLayout>
class QLabel;
class QLineEdit;
class QStackedLayout;
class QToolButton;
class QTimer;
class QHBoxLayout;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

class ScreenshotCard : public QFrame {
    Q_OBJECT

   public:
    explicit ScreenshotCard(QWidget* parent = nullptr);

    static constexpr QSize CardSize = { 192, 160 };
    static constexpr int ActionInset = 8;
    static constexpr int CardMargin = 6;

   public:
    void setScreenshot(const QPixmap&, const QString& name);
    void setSelected(bool selected);
    void setHovered(bool hovered);
    void setCurrent(bool current);

    void setIndex(const QModelIndex& index);

    void startRename();
    void cancelRename() { endRename(false); }

   protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

   signals:
    void renameRequested(const QPersistentModelIndex& index, const QString& newName);
    void deleteRequested(const QPersistentModelIndex& index);

   private:
    void endRename(bool commit);
    bool eventFilter(QObject* watched, QEvent* event) override;
    void updateActionButtons();
    QSize thumbnailArea() const;
    void updateThumbnail();

   private:
    QPixmap m_thumbnail;  // unscaled source from the model
    QLabel* m_screenshot;
    QLabel* m_label;
    QLineEdit* m_edit;
    QPointer<QStackedLayout> m_labelStack;
    QVBoxLayout m_layout;

    bool m_selected = false, m_hovered = false, m_current = false;

    QPersistentModelIndex m_index;
    bool m_editing = false;

    QFrame* m_actions;  // container, overlays the thumbnail
    QGraphicsOpacityEffect* m_actionsOpacity = nullptr;
    QPropertyAnimation* m_actionsFade = nullptr;

    QToolButton* m_rename;
    QToolButton* m_delete;
    QTimer* m_hoverTimer;
};

class ScreenshotItemDelegate : public WidgetDelegate {
    Q_OBJECT

   public:
    explicit ScreenshotItemDelegate(QObject* parent);

   signals:
    void renameRequested(const QPersistentModelIndex& index, const QString& newName);
    void deleteRequested(const QPersistentModelIndex& index);

   public:
    void destroyEditor(QWidget* editor, const QModelIndex& /*index*/) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void paintItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void updateEditorState(QWidget* editor, const QModelIndex& index) const override;

    void startRename(const QModelIndex& index);
};
