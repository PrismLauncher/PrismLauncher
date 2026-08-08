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

#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QPixmap>

class CapeList : public QAbstractListModel {
    Q_OBJECT
   public:
    explicit CapeList(QObject* parent = nullptr);

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    void addCape(QString id, QString alias, QImage image);
    bool hasCape(const QString& id) const;
    QString idAt(const QModelIndex& index) const;
    QModelIndex indexOf(const QString& id) const;

    void setElytraVisible(bool visible);
    void refresh();

   private:
    static QImage makePreview(const QImage& image, bool elytra);
    static QPixmap makeNoCapePreview();
    static QPixmap makeEmptyPreview();

   private:
    struct CapeEntry {
        QString id;
        QString alias;
        QPixmap previewNormal;
        QPixmap previewElytra;
    };
    QList<CapeEntry> m_capes;
    bool m_elytra = false;
};
