#pragma once
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QString>

struct FMLlib {
    QString filename;
    QString checksum;
};

struct VersionFilterData {
    VersionFilterData();
    // mapping between minecraft versions and FML libraries required
    QMap<QString, QList<FMLlib>> fmlLibsMapping;
    // set of minecraft versions for which using forge installers is blacklisted
    QSet<QString> forgeInstallerBlacklist;
    // no new versions below this date will be accepted from Mojang servers
    QDateTime legacyCutoffDate;
    // Libraries that belong to LWJGL
    QSet<QString> lwjglWhitelist;
    // release date of first version to require Java 8 (17w13a)
    QDateTime java8BeginsDate;
    // release data of first version to require Java 16 (21w19a)
    QDateTime java16BeginsDate;
    // release data of first version to require Java 17 (1.18 Pre Release 2)
    QDateTime java17BeginsDate;
};
extern VersionFilterData g_VersionFilterData;
