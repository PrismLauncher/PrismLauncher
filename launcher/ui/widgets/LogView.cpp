// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "LogView.h"
#include <QTextBlock>
#include <QScrollBar>

LogView::LogView(QWidget* parent) : QPlainTextEdit(parent)
{
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_defaultFormat = new QTextCharFormat(currentCharFormat());
}

LogView::~LogView()
{
    delete hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_defaultFormat;
}

void LogView::setWordWrap(bool wrapping)
{
    if(wrapping)
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
    }
    else
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setLineWrapMode(QPlainTextEdit::NoWrap);
    }
}

void LogView::setModel(QAbstractItemModel* model)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
    {
        disconnect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::modelReset, this, &LogView::repopulate);
        disconnect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsInserted, this, &LogView::rowsInserted);
        disconnect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogView::rowsAboutToBeInserted);
        disconnect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsRemoved, this, &LogView::rowsRemoved);
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model = model;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
    {
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::modelReset, this, &LogView::repopulate);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsInserted, this, &LogView::rowsInserted);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogView::rowsAboutToBeInserted);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::rowsRemoved, this, &LogView::rowsRemoved);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model, &QAbstractItemModel::destroyed, this, &LogView::modelDestroyed);
    }
    repopulate();
}

QAbstractItemModel * LogView::model() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model;
}

void LogView::modelDestroyed(QObject* model)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model == model)
    {
        setModel(nullptr);
    }
}

void LogView::repopulate()
{
    auto doc = document();
    doc->clear();
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model)
    {
        return;
    }
    rowsInserted(QModelIndex(), 0, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->rowCount() - 1);
}

void LogView::rowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
    QScrollBar *bar = verticalScrollBar();
    int max_bar = bar->maximum();
    int val_bar = bar->value();
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scroll)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scroll = (max_bar - val_bar) <= 1;
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scroll = val_bar == max_bar;
    }
}

void LogView::rowsInserted(const QModelIndex& parent, int first, int last)
{
    for(int i = first; i <= last; i++)
    {
        auto idx = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->index(i, 0, parent);
        auto text = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->data(idx, Qt::DisplayRole).toString();
        QTextCharFormat format(*hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_defaultFormat);
        auto font = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->data(idx, Qt::FontRole);
        if(font.isValid())
        {
            format.setFont(font.value<QFont>());
        }
        auto fg = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->data(idx, Qt::ForegroundRole);
        if(fg.isValid())
        {
            format.setForeground(fg.value<QColor>());
        }
        auto bg = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->data(idx, Qt::BackgroundRole);
        if(bg.isValid())
        {
            format.setBackground(bg.value<QColor>());
        }
        auto workCursor = textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(text, format);
        workCursor.insertBlock();
    }
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scroll && !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scrolling)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scrolling = true;
        QMetaObject::invokeMethod( this, "scrollToBottom", Qt::QueuedConnection);
    }
}

void LogView::rowsRemoved(const QModelIndex& parent, int first, int last)
{
    // TODO: some day... maybe
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void LogView::scrollToBottom()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scrolling = false;
    verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
}

void LogView::findNext(const QString& what, bool reverse)
{
    find(what, reverse ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));
}
