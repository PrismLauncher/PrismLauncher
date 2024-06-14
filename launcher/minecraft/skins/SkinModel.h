// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include <QDir>
#include <QJsonObject>
#include <QPixmap>

class SkinModel {
   public:
    enum Model { CLASSIC, SLIM };

    SkinModel() = default;
    SkinModel(QString path);
    SkinModel(QDir skinDir, QJsonObject obj);
    virtual ~SkinModel() = default;

    QString name() const;
    QString getModelString() const;
    bool isValid() const;
    QString getPath() const { return m_path; }
    QPixmap getTexture() const { return m_texture; }
    QString getCapeId() const { return m_cape_id; }
    Model getModel() const { return m_model; }
    QString getURL() const { return m_url; }

    bool rename(QString newName);
    void setCapeId(QString capeID) { m_cape_id = capeID; }
    void setModel(Model model) { m_model = model; }
    void setURL(QString url) { m_url = url; }
    void refresh() { m_texture = QPixmap(m_path); }

    QJsonObject toJSON() const;

   private:
    QString m_path;
    QPixmap m_texture;
    QString m_cape_id;
    Model m_model;
    QString m_url;
};