#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVector>

namespace ModPlatform {

enum class Provider{
    MODRINTH,
    FLAME
};

class ProviderCapabilities {
   public:
    static QString hashType(Provider p) 
    {
        switch(p){
        case Provider::MODRINTH:
            return "sha256";
        case Provider::FLAME:
            return "murmur2";
        }
        return "";
    }
    static const char* providerName(Provider p) 
    {
        switch(p){
        case Provider::MODRINTH:
            return "modrinth";
        case Provider::FLAME:
            return "curseforge";
        }
        return "";
    }
};

struct ModpackAuthor {
    QString name;
    QString url;
};

struct IndexedVersion {
    QVariant addonId;
    QVariant fileId;
    QString version;
    QVector<QString> mcVersion;
    QString downloadUrl;
    QString date;
    QString fileName;
    QVector<QString> loaders = {};
};

struct IndexedPack {
    QVariant addonId;
    Provider provider;
    QString name;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;

    bool versionsLoaded = false;
    QVector<IndexedVersion> versions;
};

}  // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::IndexedPack)
Q_DECLARE_METATYPE(ModPlatform::Provider)
