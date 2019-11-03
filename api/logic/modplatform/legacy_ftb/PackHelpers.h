#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

namespace LegacyFTB {

//Header for structs etc...
enum class PackType
{
    Public,
    ThirdParty,
    Private
};

struct Modpack
{
    QString name;
    QString description;
    QString author;
    QStringList oldVersions;
    QString currentVersion;
    QString mcVersion;
    QString mods;
    QString logo;

    //Technical data
    QString dir;
    QString file; //<- Url in the xml, but doesn't make much sense

    bool bugged = false;
    bool broken = false;

    PackType type;
    QString packCode;
};

typedef QList<Modpack> ModpackList;

}

//We need it for the proxy model
Q_DECLARE_METATYPE(LegacyFTB::Modpack)
