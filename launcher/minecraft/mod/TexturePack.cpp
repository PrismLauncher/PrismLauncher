#include "TexturePack.h"

#include <QDebug>
#include <QMap>
#include <QRegularExpression>

#include "minecraft/mod/tasks/LocalTexturePackParseTask.h"

void TexturePack::setDescription(QString new_description)
{
    QMutexLocker locker(&m_data_lock);

    m_description = new_description;
}

void TexturePack::setImage(QImage new_image)
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_pack_image_cache_key.key.isValid())
        QPixmapCache::remove(m_pack_image_cache_key.key);

    m_pack_image_cache_key.key = QPixmapCache::insert(QPixmap::fromImage(new_image));
    m_pack_image_cache_key.was_ever_used = true;
}

QPixmap TexturePack::image(QSize size)
{
    QPixmap cached_image;
    if (QPixmapCache::find(m_pack_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size);
    }

    // No valid image we can get
    if (!m_pack_image_cache_key.was_ever_used)
        return {};

    // Imaged got evicted from the cache. Re-process it and retry.
    TexturePackUtils::process(*this);
    return image(size);
}
