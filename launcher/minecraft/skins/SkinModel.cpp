// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
 *  Copyright (c) 2025 Rinth, Inc.
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
#include <QPainter>
#include <array>
#include <span>

#include "FileSystem.h"

namespace {
void setAlpha(QImage& image, const QRect& region, const int alpha)
{
    for (int y = region.top(); y < region.bottom(); ++y) {
        auto line = std::span(reinterpret_cast<QRgb*>(image.scanLine(y)), image.width());
        for (int x = region.left(); x < region.right(); ++x) {
            QRgb pixel = line[x];
            line[x] = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), alpha);
        }
    }
}

void doNotchTransparencyHack(QImage& image)
{
    for (int y = 0; y < 32; y++) {
        auto line = std::span(reinterpret_cast<QRgb*>(image.scanLine(y)), image.width());
        for (int x = 32; x < 64; x++) {
            if (qAlpha(line[x]) < 128) {
                return;
            }
        }
    }

    setAlpha(image, { 32, 0, 32, 32 }, 0);
}

QImage improveSkin(QImage skin)
{
    int height = skin.height();
    int width = skin.width();
    if (width != 64 || (height != 32 && height != 64)) {  // this is no minecraft skin
        return skin;
    }
    // It seems some older skins may use this format, which can't be drawn onto
    // https://github.com/PrismLauncher/PrismLauncher/issues/4032
    // https://doc.qt.io/qt-6/qpainter.html#begin
    if (skin.format() <= QImage::Format_Indexed8 || !skin.hasAlphaChannel()) {
        skin = skin.convertToFormat(QImage::Format_ARGB32);
    }

    auto isLegacy = height == 32;  // old format
    if (isLegacy) {
        auto newSkin = QImage(QSize(64, 64), skin.format());
        newSkin.fill(Qt::transparent);
        QPainter p(&newSkin);
        p.drawImage(0, 0, skin);

        auto copyRect = [&p, &newSkin](int startX, int startY, int offsetX, int offsetY, int sizeX, int sizeY) {
            QImage region = newSkin.copy(startX, startY, sizeX, sizeY);
            region = region.flipped(Qt::Horizontal);

            p.drawImage(startX + offsetX, startY + offsetY, region);
        };
        struct FaceRect {
            int x;
            int y;
            int offsetX;
            int offsetY;
            int width;
            int height;
        };
        static constexpr std::array s_faces = {
            FaceRect{ .x = 4, .y = 16, .offsetX = 16, .offsetY = 32, .width = 4, .height = 4 },
            FaceRect{ .x = 8, .y = 16, .offsetX = 16, .offsetY = 32, .width = 4, .height = 4 },
            FaceRect{ .x = 0, .y = 20, .offsetX = 24, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 4, .y = 20, .offsetX = 16, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 8, .y = 20, .offsetX = 8, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 12, .y = 20, .offsetX = 16, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 44, .y = 16, .offsetX = -8, .offsetY = 32, .width = 4, .height = 4 },
            FaceRect{ .x = 48, .y = 16, .offsetX = -8, .offsetY = 32, .width = 4, .height = 4 },
            FaceRect{ .x = 40, .y = 20, .offsetX = 0, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 44, .y = 20, .offsetX = -8, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 48, .y = 20, .offsetX = -16, .offsetY = 32, .width = 4, .height = 12 },
            FaceRect{ .x = 52, .y = 20, .offsetX = -8, .offsetY = 32, .width = 4, .height = 12 },
        };

        for (const auto& face : s_faces) {
            copyRect(face.x, face.y, face.offsetX, face.offsetY, face.width, face.height);
        }
        doNotchTransparencyHack(newSkin);
        skin = newSkin;
    }
    static constexpr std::array s_opaqueParts = {
        QRect{ 0, 0, 32, 16 },
        QRect{ 0, 16, 64, 16 },
        QRect{ 16, 48, 32, 16 },
    };

    for (const auto& p : s_opaqueParts) {
        setAlpha(skin, p, 255);
    }
    return skin;
}

QImage getSkin(const QString& path)
{
    return improveSkin(QImage(path));
}

QImage generatePreviews(const QImage& texture, bool slim)
{
    QImage preview(36, 36, QImage::Format_ARGB32);
    preview.fill(Qt::transparent);
    QPainter paint(&preview);

    // head
    paint.drawImage(4, 2, texture.copy(8, 8, 8, 8));
    paint.drawImage(4, 2, texture.copy(40, 8, 8, 8));
    // torso
    paint.drawImage(4, 10, texture.copy(20, 20, 8, 12));
    paint.drawImage(4, 10, texture.copy(20, 36, 8, 12));
    // right leg
    paint.drawImage(4, 22, texture.copy(4, 20, 4, 12));
    paint.drawImage(4, 22, texture.copy(4, 36, 4, 12));
    // left leg
    paint.drawImage(8, 22, texture.copy(20, 52, 4, 12));
    paint.drawImage(8, 22, texture.copy(4, 52, 4, 12));

    auto armWidth = slim ? 3 : 4;
    auto armPosX = slim ? 1 : 0;
    // right arm
    paint.drawImage(armPosX, 10, texture.copy(44, 20, armWidth, 12));
    paint.drawImage(armPosX, 10, texture.copy(44, 36, armWidth, 12));
    // left arm
    paint.drawImage(12, 10, texture.copy(36, 52, armWidth, 12));
    paint.drawImage(12, 10, texture.copy(52, 52, armWidth, 12));

    // back
    // head
    paint.drawImage(24, 2, texture.copy(24, 8, 8, 8));
    paint.drawImage(24, 2, texture.copy(56, 8, 8, 8));
    // torso
    paint.drawImage(24, 10, texture.copy(32, 20, 8, 12));
    paint.drawImage(24, 10, texture.copy(32, 36, 8, 12));
    // right leg
    paint.drawImage(24, 22, texture.copy(12, 20, 4, 12));
    paint.drawImage(24, 22, texture.copy(12, 36, 4, 12));
    // left leg
    paint.drawImage(28, 22, texture.copy(28, 52, 4, 12));
    paint.drawImage(28, 22, texture.copy(12, 52, 4, 12));

    // right arm
    paint.drawImage(armPosX + 20, 10, texture.copy(48 + armWidth, 20, armWidth, 12));
    paint.drawImage(armPosX + 20, 10, texture.copy(48 + armWidth, 36, armWidth, 12));
    // left arm
    paint.drawImage(32, 10, texture.copy(40 + armWidth, 52, armWidth, 12));
    paint.drawImage(32, 10, texture.copy(56 + armWidth, 52, armWidth, 12));

    return preview;
}
}  // namespace

SkinModel::SkinModel(const QString& path) : m_path(path), m_texture(getSkin(path))
{
    m_preview = generatePreviews(m_texture, false);
}

SkinModel::SkinModel(const QDir& skinDir, QJsonObject obj) : m_capeId(obj.value("capeId").toString()), m_url(obj.value("url").toString())
{
    auto name = obj["name"].toString();

    if (auto model = obj["model"].toString(); model == "SLIM") {
        m_model = Model::SLIM;
    }
    m_path = skinDir.absoluteFilePath(name) + ".png";
    m_texture = getSkin(m_path);
    m_preview = generatePreviews(m_texture, m_model == Model::SLIM);
}

QString SkinModel::name() const
{
    return QFileInfo(m_path).completeBaseName();
}

bool SkinModel::rename(const QString& newName)
{
    auto info = QFileInfo(m_path);
    auto newPath = FS::PathCombine(info.absolutePath(), newName + ".png");
    if (QFileInfo::exists(newPath)) {
        return false;
    }
    m_path = newPath;
    return FS::move(info.absoluteFilePath(), m_path);
}

QJsonObject SkinModel::toJSON() const
{
    QJsonObject obj;
    obj["name"] = name();
    obj["capeId"] = m_capeId;
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
void SkinModel::refresh()
{
    m_texture = getSkin(m_path);
    m_preview = generatePreviews(m_texture, m_model == Model::SLIM);
}
void SkinModel::setModel(Model model)
{
    m_model = model;
    m_preview = generatePreviews(m_texture, m_model == Model::SLIM);
}
