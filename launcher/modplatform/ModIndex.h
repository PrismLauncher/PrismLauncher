#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVector>

namespace ModPlatform {

enum class Provider {
    MODRINTH,
    FLAME
};

class ProviderCapabilities {
   public:
    auto name(Provider) -> const char*;
    auto readableName(Provider) -> QString;
    auto hashType(Provider) -> QStringList;
    auto hash(Provider, QByteArray&, QString type = "") -> QByteArray;
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
    QString hash_type;
    QString hash;
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
