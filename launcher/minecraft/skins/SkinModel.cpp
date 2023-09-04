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
    auto skinImage = Json::ensureString(obj, "skinImage");
    if (!skinImage.isEmpty()) {  // minecraft skin model
        skinImage = skinImage.mid(22);
        m_texture.loadFromData(QByteArray::fromBase64(skinImage.toUtf8()), "PNG");
        auto textureId = Json::ensureString(obj, "textureId");
        if (name.isEmpty()) {
            name = textureId;
        }
        if (Json::ensureBoolean(obj, "slim", false)) {
            m_model = Model::SLIM;
        }
    } else {
        if (auto model = Json::ensureString(obj, "model"); model == "SLIM") {
            m_model = Model::SLIM;
        }
    }
    m_path = skinDir.absoluteFilePath(name) + ".png";
    if (!QFileInfo(m_path).exists() && isValid()) {
        m_texture.save(m_path, "PNG");
    } else {
        m_texture = QPixmap(m_path);
    }
}

QString SkinModel::name() const
{
    return QFileInfo(m_path).baseName();
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

QPixmap SkinModel::renderFrontBody() const
{
    auto isSlim = m_model == SLIM;
    auto slimOffset = isSlim ? 1 : 0;
    auto isOldSkin = m_texture.height() < 64;

    auto head = m_texture.copy(QRect(8, 8, 16, 16));
    auto torso = m_texture.copy(QRect(20, 20, 28, 32));
    auto rightArm = m_texture.copy(QRect(44, 20, 48 - slimOffset, 32));
    auto rightLeg = m_texture.copy(QRect(4, 20, 8, 32));
    QPixmap leftArm, leftLeg;

    if (isOldSkin) {
        leftArm = rightArm.transformed(QTransform().scale(-1, 1));
        leftLeg = rightLeg.transformed(QTransform().scale(-1, 1));
    } else {
        leftArm = m_texture.copy(QRect(36, 52, 40 - slimOffset, 64));
        leftLeg = m_texture.copy(QRect(20, 52, 24, 64));
    }
    QPixmap output(16, 32);
    output.fill(Qt::black);
    QPainter p;
    if (!p.begin(&output))
        return {};
    p.drawPixmap(QPoint(4, 0), head);
    p.drawPixmap(QPoint(4, 8), torso);
    p.drawPixmap(QPoint(12, 8), leftArm);
    p.drawPixmap(QPoint(slimOffset, 8), rightArm);
    p.drawPixmap(QPoint(8, 20), leftLeg);
    p.drawPixmap(QPoint(4, 20), leftArm);

    return output.scaled(128, 128, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
}