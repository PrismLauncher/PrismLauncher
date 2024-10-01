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

#include "SkinModel.h"
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTransform>

#include "FileSystem.h"
#include "Json.h"

SkinModel::SkinModel(QString path) : m_path(path), m_texture(path), m_model(Model::CLASSIC) {}

SkinModel::SkinModel(QDir skinDir, QJsonObject obj)
    : m_cape_id(Json::ensureString(obj, "capeId")), m_model(Model::CLASSIC), m_url(Json::ensureString(obj, "url"))
{
    auto name = Json::ensureString(obj, "name");

    if (auto model = Json::ensureString(obj, "model"); model == "SLIM") {
        m_model = Model::SLIM;
    }
    m_path = skinDir.absoluteFilePath(name) + ".png";
    m_texture = QPixmap(m_path);
}

QString SkinModel::name() const
{
    return QFileInfo(m_path).completeBaseName();
}

bool SkinModel::rename(QString newName)
{
    auto info = QFileInfo(m_path);
    m_path = FS::PathCombine(info.absolutePath(), newName + ".png");
    return FS::move(info.absoluteFilePath(), m_path);
}

QJsonObject SkinModel::toJSON() const
{
    QJsonObject obj;
    obj["name"] = name();
    obj["capeId"] = m_cape_id;
    obj["url"] = m_url;
    obj["model"] = getModelString();
    return obj;
}

QString SkinModel::getModelString() const
{
    switch (m_model) {
        case CLASSIC:
            return "CLASSIC";
        case SLIM:
            return "SLIM";
    }
    return {};
}

bool SkinModel::isValid() const
{
    return !m_texture.isNull() && (m_texture.size().height() == 32 || m_texture.size().height() == 64) && m_texture.size().width() == 64;
}
