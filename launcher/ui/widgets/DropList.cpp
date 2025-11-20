// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
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
 */

#include "DropList.h"

#include <QDropEvent>
#include <QMimeData>

DropList::DropList(QWidget* parent) : QListWidget(parent)
{
    setAcceptDrops(true);
}

void DropList::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void DropList::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void DropList::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void DropList::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData) {
        return;
    }

    if (mimeData->hasUrls()) {
        auto urls = mimeData->urls();
        emit droppedURLs(urls);
    }

    event->acceptProposedAction();
}

void DropList::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit deleteKeyPressed();
    } else {
        QListWidget::keyPressEvent(event);
    }
}