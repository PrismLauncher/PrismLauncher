// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vivek Kushwaha <notvivekkushwaha@gmail.com>
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

#include "ProfileCheckStateDelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QSortFilterProxyModel>

#include "minecraft/mod/ModFolderModel.h"

ProfileCheckStateDelegate::ProfileCheckStateDelegate(
    const QMap<QString, QSet<QString>>* profileStates,
    const QString* currentProfile,
    QSortFilterProxyModel* filterModel,
    ModFolderModel* model,
    QObject* parent)
    : QStyledItemDelegate(parent)
    , m_profileStates(profileStates)
    , m_currentProfile(currentProfile)
    , m_filterModel(filterModel)
    , m_model(model)
{}

QString ProfileCheckStateDelegate::resolveModId(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid())
        return {};
    QModelIndex sourceIndex = m_filterModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid())
        return {};
    int row = sourceIndex.row();
    if (row < 0 || row >= m_model->rowCount())
        return {};
    return m_model->at(row).mod_id();
}

void ProfileCheckStateDelegate::initStyleOption(QStyleOptionViewItem* option,
                                               const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() == ModFolderModel::ActiveColumn) {
        const QString modId = resolveModId(index);
        const bool checked = !m_currentProfile->isEmpty() &&
                             m_profileStates->contains(*m_currentProfile) &&
                             (*m_profileStates)[*m_currentProfile].contains(modId);
        option->features |= QStyleOptionViewItem::HasCheckIndicator;
        option->checkState = checked ? Qt::Checked : Qt::Unchecked;
        if (checked) {
            option->state |= QStyle::State_On;
            option->state &= ~QStyle::State_Off;
        } else {
            option->state |= QStyle::State_Off;
            option->state &= ~QStyle::State_On;
        }
    }
}

void ProfileCheckStateDelegate::paint(QPainter* painter,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
}

bool ProfileCheckStateDelegate::editorEvent(QEvent* event,
                                            QAbstractItemModel* /*model*/,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index)
{
    if (index.column() != ModFolderModel::ActiveColumn)
        return false;
    if (m_currentProfile->isEmpty())
        return false;

    bool isClick = false;
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && option.rect.contains(me->pos()))
            isClick = true;
    } else if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Space || ke->key() == Qt::Key_Return)
            isClick = true;
    }

    if (!isClick)
        return false;

    const QString modId = resolveModId(index);
    if (modId.isEmpty())
        return false;

    bool currentlyEnabled = (*m_profileStates)[*m_currentProfile].contains(modId);
    emit membershipToggled(modId, !currentlyEnabled);
    return true;
}
