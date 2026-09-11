// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Guanran Wang <guanran928@outlook.com>
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

#include "CapeList.h"

#include <QPainter>
#include <QPalette>
#include <utility>

CapeList::CapeList(QObject* parent) : QAbstractListModel(parent) {}

QVariant CapeList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_capes.size()) {
        return {};
    }

    const auto& entry = m_capes[index.row()];
    switch (role) {
        case Qt::DecorationRole:
            return m_elytra ? entry.previewElytra : entry.previewNormal;
        case Qt::DisplayRole:
            return entry.alias;
        case Qt::UserRole:
            return entry.id;
        default:
            return {};
    }
}

int CapeList::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_capes.size());
}

void CapeList::addCape(QString id, QString alias, QImage image)
{
    beginInsertRows(QModelIndex(), int(m_capes.size()), int(m_capes.size()));
    CapeEntry entry{ .id = std::move(id), .alias = std::move(alias) };
    if (image.isNull()) {
        if (entry.id.isEmpty()) {
            entry.previewNormal = makeNoCapePreview();
            entry.previewElytra = makeNoCapePreview();
        } else {
            entry.previewNormal = makeEmptyPreview();
            entry.previewElytra = makeEmptyPreview();
        }
    } else {
        entry.previewNormal = QPixmap::fromImage(makePreview(image, false));
        entry.previewElytra = QPixmap::fromImage(makePreview(image, true));
    }
    m_capes.append(std::move(entry));
    endInsertRows();
}

bool CapeList::hasCape(const QString& id) const
{
    return indexOf(id).isValid();
}

QString CapeList::idAt(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_capes.size()) {
        return {};
    }
    return m_capes[index.row()].id;
}

QModelIndex CapeList::indexOf(const QString& id) const
{
    for (int i = 0; i < m_capes.size(); i++) {
        if (m_capes[i].id == id) {
            return index(i);
        }
    }
    return {};
}

void CapeList::setElytraVisible(bool visible)
{
    if (m_elytra == visible) {
        return;
    }
    m_elytra = visible;
    refresh();
}

void CapeList::refresh()
{
    if (m_capes.isEmpty()) {
        return;
    }
    emit dataChanged(index(0), index(int(m_capes.size()) - 1), { Qt::DecorationRole });
}

QImage CapeList::makePreview(const QImage& capeImage, bool elytra)
{
    if (elytra) {
        auto wing = capeImage.copy(34, 2, 12, 20);
        const QImage mirrored = wing.mirrored(true, false);

        QImage combined((wing.width() * 2) + 1, wing.height() + 14, capeImage.format());
        combined.fill(Qt::transparent);

        QPainter painter(&combined);
        painter.drawImage(0, 7, wing);
        painter.drawImage(wing.width() + 1, 7, mirrored);
        painter.end();

        // make sure to output same size as normal cape img
        QImage canvas(40, 64, capeImage.format());
        canvas.fill(Qt::transparent);
        const QSize target = combined.size().scaled(40, 64, Qt::KeepAspectRatio);
        QPainter canvasPainter(&canvas);
        canvasPainter.drawImage(QRect((40 - target.width()) / 2, (64 - target.height()) / 2, target.width(), target.height()), combined);
        canvasPainter.end();
        return canvas;
    }
    return capeImage.copy(1, 1, 10, 16).scaled(40, 64, Qt::KeepAspectRatio);
}

QPixmap CapeList::makeNoCapePreview()
{
    QPixmap preview(40, 64);
    preview.fill(Qt::transparent);

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor textColor = QPalette().color(QPalette::Disabled, QPalette::Text);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.setPen(QPen(textColor, 2));
    painter.drawRect(preview.rect().adjusted(1, 1, -1, -1));
    painter.drawLine(QPointF(0, 0), QPointF(preview.width(), preview.height()));
    painter.drawLine(QPointF(preview.width(), 0), QPointF(0, preview.height()));
    painter.end();

    return preview;
}

QPixmap CapeList::makeEmptyPreview()
{
    QPixmap preview(40, 64);
    preview.fill(Qt::transparent);

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor textColor = QPalette().color(QPalette::Disabled, QPalette::Text);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.setPen(QPen(textColor, 2));
    painter.drawRect(preview.rect().adjusted(1, 1, -1, -1));
    painter.end();

    return preview;
}
