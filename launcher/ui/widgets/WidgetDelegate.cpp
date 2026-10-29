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
    // don't re-parent keep editor parented to viewport
    m_pool.append(editor);  // store for later reuse
}
