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

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QList>
#include <QPointer>
#include <QSet>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include <QWidget>
class QPainter;

#include <functional>

class WidgetDelegate : public QStyledItemDelegate {
    Q_OBJECT

   public:
    using WidgetFactory = std::function<QWidget*(QWidget* parent)>;

    explicit WidgetDelegate(WidgetFactory factory, QObject* parent = nullptr);
    ~WidgetDelegate() override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const override;
    void destroyEditor(QWidget* editor, const QModelIndex& /*index*/) const override;

    // when called records the index into `m_painted`
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // stores m_view, MUST be called after creation and before first paint to work correctly
    void attachToView(QAbstractItemView* view);

    // does the actual paint, implement down the chain
    virtual void paintItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const = 0;

    // refresh non modal data (selection, hover, etc) on open editors
    void refreshEditors() const;

    // set editor state (selected, current)
    virtual void updateEditorState(QWidget* editor, const QModelIndex& index) const;

    QAbstractItemView* view() const { return m_view; }

   private:
    void scheduleReconcile() const;
    // synchronizes the open editors with the visible(painted) indexes
    void reconcile() const;

   private:
    WidgetFactory m_factory;
    mutable QList<QPointer<QWidget>> m_pool;

    // list of indexes on which a paint was called
    // effectively the list of visible indexes
    mutable QSet<QPersistentModelIndex> m_painted;
    // set of indexes with open editors
    mutable QSet<QPersistentModelIndex> m_open;
    mutable bool m_syncRequested = false;

    QPointer<QAbstractItemView> m_view;
};
