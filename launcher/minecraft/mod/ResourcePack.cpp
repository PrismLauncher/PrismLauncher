#include "ResourcePack.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMap>
#include "MTPixmapCache.h"
#include "Version.h"

// Values taken from:
// https://minecraft.wiki/w/Pack_format#List_of_resource_pack_formats
static const QMap<int, std::pair<Version, Version>> s_pack_format_versions = {
    { 1, { Version("1.6.1"), Version("1.8.9") } },         { 2, { Version("1.9"), Version("1.10.2") } },
    { 3, { Version("1.11"), Version("1.12.2") } },         { 4, { Version("1.13"), Version("1.14.4") } },
    { 5, { Version("1.15"), Version("1.16.1") } },         { 6, { Version("1.16.2"), Version("1.16.5") } },
    { 7, { Version("1.17"), Version("1.17.1") } },         { 8, { Version("1.18"), Version("1.18.2") } },
    { 9, { Version("1.19"), Version("1.19.2") } },         { 11, { Version("22w42a"), Version("22w44a") } },
    { 12, { Version("1.19.3"), Version("1.19.3") } },      { 13, { Version("1.19.4"), Version("1.19.4") } },
    { 14, { Version("23w14a"), Version("23w16a") } },      { 15, { Version("1.20"), Version("1.20.1") } },
    { 16, { Version("23w31a"), Version("23w31a") } },      { 17, { Version("23w32a"), Version("23w35a") } },
    { 18, { Version("1.20.2"), Version("23w16a") } },      { 19, { Version("23w42a"), Version("23w42a") } },
    { 20, { Version("23w43a"), Version("23w44a") } },      { 21, { Version("23w45a"), Version("23w46a") } },
    { 22, { Version("1.20.3-pre1"), Version("23w51b") } }, { 24, { Version("24w03a"), Version("24w04a") } },
    { 25, { Version("24w05a"), Version("24w05b") } },      { 26, { Version("24w06a"), Version("24w07a") } },
    { 28, { Version("24w09a"), Version("24w10a") } },      { 29, { Version("24w11a"), Version("24w11a") } },
    { 30, { Version("24w12a"), Version("23w12a") } },      { 31, { Version("24w13a"), Version("1.20.5-pre3") } },
    { 32, { Version("1.20.5-pre4"), Version("1.20.6") } }, { 33, { Version("24w18a"), Version("24w20a") } },
    { 34, { Version("24w21a"), Version("1.21") } }
};

std::pair<Version, Version> ResourcePack::compatibleVersions() const
{
    if (!s_pack_format_versions.contains(m_pack_format)) {
        return { {}, {} };
    }

    return s_pack_format_versions.constFind(m_pack_format).value();
}
