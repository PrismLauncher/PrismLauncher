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

#include "WidgetDelegate.h"
#include <QMetaObject>
#include <utility>

WidgetDelegate::WidgetDelegate(WidgetDelegate::WidgetFactory factory, QObject* parent)
    : QStyledItemDelegate(parent), m_factory(std::move(factory))
{}

WidgetDelegate::~WidgetDelegate()
{
    for (const auto& widget : m_pool) {
        delete widget.data();  // delete nullptr is noop
    }
}

QWidget* WidgetDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    while (!m_pool.isEmpty()) {
        if (QWidget* widget = m_pool.takeLast()) {
            return widget;  // parent is already the viewport
        }
    }
    return m_factory(parent);
}

void WidgetDelegate::destroyEditor(QWidget* editor, const QModelIndex& /*index*/) const
{
    if (!editor) {
        return;
    }
    editor->hide();
    // keep the editor parented to the viewport, it is reused from the pool
    m_pool.append(editor);
    trimPool();
}

void WidgetDelegate::trimPool() const
{
    // evict the least recently pooled; createEditor() pops from the back
    while (m_pool.size() > MaxPooledEditors) {
        if (QWidget* oldest = m_pool.takeFirst()) {
            oldest->deleteLater();
        }
    }
}

void WidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Why this works: QT composites paint batches after they are finished and
    // recurses into painting the children of an object *after* an object's own paint event.
    // So if the editor is opened here it will be present as a child after this paint ends.
    // Thus it will be painted, composited, and show up in the same frame.

    if (!m_view) {
        // we arn't even properly set up, do a reasonable default paint.
        paintItem(painter, option, index);
        return;
    }

    if (!m_open.contains(index)) {
        // open a new editor
        // insert before open so a nested pass can't double-open
        m_open.insert(index);
        m_view->openPersistentEditor(index);
    }

    scheduleReconcile();

    if (QWidget* editor = m_view->indexWidget(index)) {
        updateEditorState(editor, index);
        return;  // don't call paintItem for this one, it will paint itself in this pass
    }

    // in theory, this is unreachable but opening can fail, e.g. the factory returning a nullptr;
    // fall back to painting the item
    paintItem(painter, option, index);
}

void WidgetDelegate::attachToView(QAbstractItemView* view)
{
    m_view = view;
}

void WidgetDelegate::scheduleReconcile() const
{
    if (m_syncRequested) {
        return;
    }
    m_syncRequested = true;
    // The cast only satisfies invokeMethod's receiver parameter. Everything reconcile()
    // touches is mutable, and the QObject itself is never modified.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    QMetaObject::invokeMethod(const_cast<WidgetDelegate*>(this), &WidgetDelegate::reconcile, Qt::QueuedConnection);
}

void WidgetDelegate::reconcile() const
{
    m_syncRequested = false;

    // no viewport? can't reconcile
    if (!m_view) {
        return;
    }

    // prune m_open of editors that are no longer open and indexes that are no longer valid
    //
    // covers the effects of  `rowsRemoved`, `ItemView::reset()`, a delegate swap, etc.
    for (auto it = m_open.begin(); it != m_open.end();) {
        if (!it->isValid() || !m_view->isPersistentEditorOpen(*it)) {
            it = m_open.erase(it);
        } else {
            ++it;
        }
    }

    // close editors that arn't visible
    const QRect viewport = m_view->viewport()->rect();
    QList<QPersistentModelIndex> toClose;
    for (const auto& index : m_open) {
        const QRect rect = m_view->visualRect(index);
        if (!rect.isValid() || !rect.intersects(viewport)) {
            toClose.append(index);
        }
    }
    for (const auto& index : toClose) {
        m_open.remove(index);
        if (m_view->isPersistentEditorOpen(index)) {
            m_view->closePersistentEditor(index);
        }
    }

    trimPool();
}

void WidgetDelegate::refreshEditors() const
{
    if (!m_view) {
        return;
    }
    for (const auto& index : m_open) {
        if (auto* editor = m_view->indexWidget(index)) {
            updateEditorState(editor, index);
        }
    }
}

void WidgetDelegate::updateEditorState(QWidget* editor, const QModelIndex& index) const {}
