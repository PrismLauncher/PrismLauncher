#pragma once

#include "Resource.h"

#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

class Version;

/* TODO:
 *
 * Store localized descriptions
 * */

class TexturePack : public Resource {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    TexturePack(QObject* parent = nullptr) : Resource(parent) {}
    TexturePack(QFileInfo file_info) : Resource(file_info) {}

    /** Gets the description of the resource pack. */
    [[nodiscard]] QString description() const { return m_description; }

    /** Gets the image of the resource pack, converted to a QPixmap for drawing, and scaled to size. */
    [[nodiscard]] QPixmap image(QSize size);

    /** Thread-safe. */
    void setDescription(QString new_description);

    /** Thread-safe. */
    void setImage(QImage new_image);

   protected:
    mutable QMutex m_data_lock;

    /** The texture pack's description, as defined in the pack.txt file.
     */
    QString m_description;

    /** The texture pack's image file cache key, for access in the QPixmapCache global instance.
     *
     *  The 'was_ever_used' state simply identifies whether the key was never inserted on the cache (true),
     *  so as to tell whether a cache entry is inexistent or if it was just evicted from the cache.
     */
    struct {
        QPixmapCache::Key key;
        bool was_ever_used = false;
    } m_pack_image_cache_key;
};
