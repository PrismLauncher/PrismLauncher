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
#include <QList>
#include <QPointer>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include <QWidget>

#include <functional>

class WidgetDelegate : public QStyledItemDelegate {
    Q_OBJECT

   public:
    using WidgetFactory = std::function<QWidget*(QWidget* parent)>;

    explicit WidgetDelegate(WidgetFactory factory, QObject* parent = nullptr);
    ~WidgetDelegate() override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const override;
    void destroyEditor(QWidget* editor, const QModelIndex& /*index*/) const override;

   private:
    WidgetFactory m_factory;
    mutable QList<QPointer<QWidget>> m_pool;
};
