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

#include "VariableSizedImageObject.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QPainter>
#include <QTextObject>
#include <memory>

#include "Application.h"

#include "net/ApiDownload.h"
#include "net/NetJob.h"

enum FormatProperties { ImageData = QTextFormat::UserProperty + 1 };

QSizeF VariableSizedImageObject::intrinsicSize(QTextDocument* doc, int posInDocument, const QTextFormat& format)
{
    Q_UNUSED(posInDocument);

    auto image = qvariant_cast<QImage>(format.property(ImageData));
    auto size = image.size();
    if (size.isEmpty())  // can't resize an empty image
        return { size };

    // calculate the new image size based on the properties
    int width = 0;
    int height = 0;
    auto widthVar = format.property(QTextFormat::ImageWidth);
    if (widthVar.isValid()) {
        width = widthVar.toInt();
    }
    auto heigthVar = format.property(QTextFormat::ImageHeight);
    if (heigthVar.isValid()) {
        height = heigthVar.toInt();
    }
    if (width != 0 && height != 0) {
        size.setWidth(width);
        size.setHeight(height);
    } else if (width != 0) {
        size.setHeight((width * size.height()) / size.width());
        size.setWidth(width);
    } else if (height != 0) {
        size.setWidth((height * size.width()) / size.height());
        size.setHeight(height);
    }

    // Get the width of the text content to make the image similar sized.
    // doc->textWidth() includes the margin, so we need to remove it.
    auto doc_width = doc->textWidth() - 2 * doc->documentMargin();

    if (size.width() > doc_width)
        size *= doc_width / (double)size.width();

    return { size };
}

void VariableSizedImageObject::drawObject(QPainter* painter,
                                          const QRectF& rect,
                                          QTextDocument* doc,
                                          int posInDocument,
                                          const QTextFormat& format)
{
    if (!format.hasProperty(ImageData)) {
        QUrl image_url{ qvariant_cast<QString>(format.property(QTextFormat::ImageName)) };
        if (m_fetching_images.contains(image_url) || image_url.isEmpty())
            return;

        auto meta = std::make_shared<ImageMetadata>();
        meta->posInDocument = posInDocument;
        meta->url = image_url;

        auto widthVar = format.property(QTextFormat::ImageWidth);
        if (widthVar.isValid()) {
            meta->width = widthVar.toInt();
        }
        auto heigthVar = format.property(QTextFormat::ImageHeight);
        if (heigthVar.isValid()) {
            meta->height = heigthVar.toInt();
        }

        loadImage(doc, meta);
        return;
    }

    auto image = qvariant_cast<QImage>(format.property(ImageData));

    painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);
    painter->drawImage(rect, image);
}

void VariableSizedImageObject::flush()
{
    m_fetching_images.clear();
}

void VariableSizedImageObject::parseImage(QTextDocument* doc, std::shared_ptr<ImageMetadata> meta)
{
    QTextCursor cursor(doc);
    cursor.setPosition(meta->posInDocument);
    cursor.setKeepPositionOnInsert(true);

    auto image_char_format = cursor.charFormat();

    image_char_format.setObjectType(QTextFormat::ImageObject);
    image_char_format.setProperty(ImageData, meta->image);
    image_char_format.setProperty(QTextFormat::ImageName, meta->url.toDisplayString());
    image_char_format.setProperty(QTextFormat::ImageWidth, meta->width);
    image_char_format.setProperty(QTextFormat::ImageHeight, meta->height);

    // Qt doesn't allow us to modify the properties of an existing object in the document.
    // So we remove the old one and add the new one with the ImageData property set.
    cursor.deleteChar();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), image_char_format);
}

void VariableSizedImageObject::loadImage(QTextDocument* doc, std::shared_ptr<ImageMetadata> meta)
{
    m_fetching_images.insert(meta->url);

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(
        m_meta_entry,
        QString("images/%1").arg(QString(QCryptographicHash::hash(meta->url.toEncoded(), QCryptographicHash::Algorithm::Sha1).toHex())));

    auto job = new NetJob(QString("Load Image: %1").arg(meta->url.fileName()), APPLICATION->network());
    job->setAskRetry(false);
    job->addNetAction(Net::ApiDownload::makeCached(meta->url, entry));

    auto full_entry_path = entry->getFullPath();
    auto source_url = meta->url;
    auto loadImage = [this, doc, full_entry_path, source_url, meta](const QImage& image) {
        doc->addResource(QTextDocument::ImageResource, source_url, image);

        meta->image = image;
        parseImage(doc, meta);

        // This size hack is needed to prevent the content from being laid out in an area smaller
        // than the total width available (weird).
        auto size = doc->pageSize();
        doc->adjustSize();
        doc->setPageSize(size);

        m_fetching_images.remove(source_url);
    };
    connect(job, &NetJob::succeeded, this, [this, full_entry_path, source_url, loadImage] {
        qDebug() << "Loaded resource at:" << full_entry_path;
        // If we flushed, don't proceed.
        if (!m_fetching_images.contains(source_url))
            return;

        QImage image(full_entry_path);
        loadImage(image);
    });
    connect(job, &NetJob::failed, this, [this, full_entry_path, source_url, loadImage](QString reason) {
        qWarning() << "Failed resource at:" << full_entry_path << " because:" << reason;
        // If we flushed, don't proceed.
        if (!m_fetching_images.contains(source_url))
            return;

        loadImage(QImage());
    });
    connect(job, &NetJob::finished, job, &NetJob::deleteLater);

    job->start();
}
