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
    { 34, { Version("24w21a"), Version("1.21") } },        { 35, { Version("24w33a"), Version("24w33a") } },
    { 36, { Version("24w34a"), Version("24w35a") } },      { 37, { Version("24w36a"), Version("24w36a") } },
    { 38, { Version("24w37a"), Version("24w37a") } },      { 39, { Version("24w38a"), Version("24w39a") } },
    { 40, { Version("24w40a"), Version("24w40a") } },      { 41, { Version("1.21.2-pre1"), Version("1.21.2-pre2") } },
    { 42, { Version("1.21.2-pre3"), Version("1.21.3") } }, { 43, { Version("24w44a"), Version("24w44a") } },
    { 44, { Version("24w45a"), Version("24w45a") } },      { 45, { Version("24w46a"), Version("24w46a") } },
    { 46, { Version("1.21.4-pre1"), Version("1.21.4") } }, { 47, { Version("25w02a"), Version("25w02a") } },
    { 48, { Version("25w03a"), Version("25w03a") } },      { 49, { Version("25w04a"), Version("25w04a") } },
    { 50, { Version("25w05a"), Version("25w05a") } },      { 51, { Version("25w06a"), Version("25w06a") } },
    { 52, { Version("25w07a"), Version("25w07a") } },      { 53, { Version("25w08a"), Version("25w09b") } },
    { 54, { Version("25w10a"), Version("25w10a") } },      { 55, { Version("1.21.5-pre1"), Version("1.21.5") } },
    { 56, { Version("25w15a"), Version("25w15a") } },      { 57, { Version("25w16a"), Version("25w16a") } },
    { 58, { Version("25w17a"), Version("25w17a") } },      { 59, { Version("25w18a"), Version("25w18a") } },
    { 60, { Version("25w19a"), Version("25w19a") } },      { 61, { Version("25w20a"), Version("25w20a") } },
    { 62, { Version("25w21a"), Version("25w21a") } },      { 63, { Version("1.21.6-pre1"), Version("1.21.7-rc1") } },
    { 64, { Version("1.21.7-rc2"), Version("1.21.8") } },  { 65.0, { Version("25w31a"), Version("25w31a") } },
    { 65, { Version("25w31a"), Version("25w31a") } },      { 65.1, { Version("25w32a"), Version("25w32a") } },
    { 65.2, { Version("25w33a"), Version("25w33a") } },    { 66.0, { Version("25w34a"), Version("25w34b") } },
    { 66, { Version("25w34a"), Version("25w34b") } },      { 67.0, { Version("25w35a"), Version("25w35a") } },
    { 67, { Version("25w35a"), Version("25w35a") } },      { 68.0, { Version("25w36a"), Version("25w36b") } },
    { 68, { Version("25w36a"), Version("25w36b") } },      { 69.0, { Version("25w37a"), Version("1.21.10") } },
    { 69, { Version("25w37a"), Version("1.21.10") } },     { 70.0, { Version("25w41a"), Version("25w41a") } }, 
    { 70, { Version("25w41a"), Version("25w41a") } }
};

std::pair<Version, Version> ResourcePack::compatibleVersions() const
{
    if (!s_pack_format_versions.contains(m_pack_format)) {
        return { {}, {} };
    }

    return s_pack_format_versions.constFind(m_pack_format).value();
}
