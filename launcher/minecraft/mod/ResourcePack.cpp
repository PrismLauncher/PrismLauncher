#include "ResourcePack.h"

#include <QDebug>
#include "launcherlog.h"
#include <QMap>
#include <QRegularExpression>

#include "Version.h"

#include "minecraft/mod/tasks/LocalResourcePackParseTask.h"

// Values taken from:
// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
static const QMap<int, std::pair<Version, Version>> s_pack_format_versions = {
    { 1, { Version("1.6.1"), Version("1.8.9") } }, { 2, { Version("1.9"), Version("1.10.2") } },
    { 3, { Version("1.11"), Version("1.12.2") } }, { 4, { Version("1.13"), Version("1.14.4") } },
    { 5, { Version("1.15"), Version("1.16.1") } }, { 6, { Version("1.16.2"), Version("1.16.5") } },
    { 7, { Version("1.17"), Version("1.17.1") } }, { 8, { Version("1.18"), Version("1.18.2") } },
    { 9, { Version("1.19"), Version("1.19.2") } },
};

void ResourcePack::setPackFormat(int new_format_id)
{
    QMutexLocker locker(&m_data_lock);

    if (!s_pack_format_versions.contains(new_format_id)) {
        qCWarning(LAUNCHER_LOG) << "Pack format '%1' is not a recognized resource pack id!";
    }

    m_pack_format = new_format_id;
}

void ResourcePack::setDescription(QString new_description)
{
    QMutexLocker locker(&m_data_lock);

    m_description = new_description;
}

void ResourcePack::setImage(QImage new_image)
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_pack_image_cache_key.key.isValid())
        QPixmapCache::remove(m_pack_image_cache_key.key);

    m_pack_image_cache_key.key = QPixmapCache::insert(QPixmap::fromImage(new_image));
    m_pack_image_cache_key.was_ever_used = true;
}

QPixmap ResourcePack::image(QSize size)
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
    ResourcePackUtils::process(*this);
    return image(size);
}

std::pair<Version, Version> ResourcePack::compatibleVersions() const
{
    if (!s_pack_format_versions.contains(m_pack_format)) {
        return { {}, {} };
    }

    return s_pack_format_versions.constFind(m_pack_format).value();
}

std::pair<int, bool> ResourcePack::compare(const Resource& other, SortType type) const
{
    auto const& cast_other = static_cast<ResourcePack const&>(other);

    switch (type) {
        default: {
            auto res = Resource::compare(other, type);
            if (res.first != 0)
                return res;
        }
        case SortType::PACK_FORMAT: {
            auto this_ver = packFormat();
            auto other_ver = cast_other.packFormat();

            if (this_ver > other_ver)
                return { 1, type == SortType::PACK_FORMAT };
            if (this_ver < other_ver)
                return { -1, type == SortType::PACK_FORMAT };
        }
    }
    return { 0, false };
}

bool ResourcePack::applyFilter(QRegularExpression filter) const
{
    if (filter.match(description()).hasMatch())
        return true;

    if (filter.match(QString::number(packFormat())).hasMatch())
        return true;

    if (filter.match(compatibleVersions().first.toString()).hasMatch())
        return true;
    if (filter.match(compatibleVersions().second.toString()).hasMatch())
        return true;

    return Resource::applyFilter(filter);
}
