#include "ResourcePack.h"

#include <QDebug>
#include <QMap>

#include "Version.h"

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
        qCritical() << "Error: Pack format '%1' is not a recognized resource pack id.";
        return;
    }

    m_pack_format = new_format_id;
}

void ResourcePack::setDescription(QString new_description)
{
    QMutexLocker locker(&m_data_lock);

    m_description = new_description;
}

std::pair<Version, Version> ResourcePack::compatibleVersions() const
{
    if (!s_pack_format_versions.contains(m_pack_format)) {
        // Not having a valid pack format is fine if we didn't yet parse the .mcmeta file,
        // but if we did and we still don't have a valid pack format, that's a bit concerning.
        Q_ASSERT(!isResolved());

        return {{}, {}};
    }

    return s_pack_format_versions.constFind(m_pack_format).value();
}
