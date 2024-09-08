// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include <QObject>
#include <QString>
#include <QTextObjectInterface>
#include <QUrl>
#include <memory>

/** Custom image text object to be used instead of the normal one in ProjectDescriptionPage.
 *
 *  Why? Because we want to re-scale images dynamically based on the document's size, in order to
 *  not have images being weirdly cropped out in different resolutions.
 */
class VariableSizedImageObject final : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

    struct ImageMetadata {
        int posInDocument;
        QUrl url;
        QImage image;
        int width;
        int height;
    };

   public:
    QSizeF intrinsicSize(QTextDocument* doc, int posInDocument, const QTextFormat& format) override;
    void drawObject(QPainter* painter, const QRectF& rect, QTextDocument* doc, int posInDocument, const QTextFormat& format) override;

    void setMetaEntry(QString meta_entry) { m_meta_entry = meta_entry; }

   public slots:
    /** Stops all currently loading images from modifying the document.
     *
     *  This does not stop the ongoing network tasks, it only prevents their result
     *  from impacting the document any further.
     */
    void flush();

   private:
    /** Adds the image to the document, in the given position.
     */
    void parseImage(QTextDocument* doc, std::shared_ptr<ImageMetadata> meta);

    /** Loads an image from an external source, and adds it to the document.
     *
     *  This uses m_meta_entry to cache the image.
     */
    void loadImage(QTextDocument* doc, std::shared_ptr<ImageMetadata> meta);

   private:
    QString m_meta_entry;

    QSet<QUrl> m_fetching_images;
};
