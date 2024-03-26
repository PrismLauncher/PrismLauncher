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

#include "Application.h"

#include "net/ApiDownload.h"
#include "net/NetJob.h"

enum FormatProperties { ImageData = QTextFormat::UserProperty + 1 };

QSizeF VariableSizedImageObject::intrinsicSize(QTextDocument* doc, int posInDocument, const QTextFormat& format)
{
    Q_UNUSED(posInDocument);

    auto image = qvariant_cast<QImage>(format.property(ImageData));
    auto size = image.size();

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
        if (m_fetching_images.contains(image_url))
            return;

        loadImage(doc, image_url, posInDocument);
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

void VariableSizedImageObject::parseImage(QTextDocument* doc, QImage image, int posInDocument)
{
    QTextCursor cursor(doc);
    cursor.setPosition(posInDocument);
    cursor.setKeepPositionOnInsert(true);

    auto image_char_format = cursor.charFormat();

    image_char_format.setObjectType(QTextFormat::ImageObject);
    image_char_format.setProperty(ImageData, image);

    // Qt doesn't allow us to modify the properties of an existing object in the document.
    // So we remove the old one and add the new one with the ImageData property set.
    cursor.deleteChar();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), image_char_format);
}

void VariableSizedImageObject::loadImage(QTextDocument* doc, const QUrl& source, int posInDocument)
{
    m_fetching_images.insert(source);

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(
        m_meta_entry,
        QString("images/%1").arg(QString(QCryptographicHash::hash(source.toEncoded(), QCryptographicHash::Algorithm::Sha1).toHex())));

    auto job = new NetJob(QString("Load Image: %1").arg(source.fileName()), APPLICATION->network());
    job->addNetAction(Net::ApiDownload::makeCached(source, entry));

    auto full_entry_path = entry->getFullPath();
    auto source_url = source;
    auto loadImage = [this, doc, full_entry_path, source_url, posInDocument](const QImage& image) {
        doc->addResource(QTextDocument::ImageResource, source_url, image);

        parseImage(doc, image, posInDocument);

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
