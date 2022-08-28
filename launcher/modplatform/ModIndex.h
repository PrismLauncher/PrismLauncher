// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 3.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVector>

class QIODevice;

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
    auto hash(Provider, QIODevice*, QString type = "") -> QString;
};

struct ModpackAuthor {
    QString name;
    QString url;
};

struct DonationData {
    QString id;
    QString platform;
    QString url;
};

struct IndexedVersion {
    QVariant addonId;
    QVariant fileId;
    QString version;
    QString version_number = {};
    QStringList mcVersion;
    QString downloadUrl;
    QString date;
    QString fileName;
    QStringList loaders = {};
    QString hash_type;
    QString hash;
    bool is_preferred = true;
    QString changelog;
};

struct ExtraPackData {
    QList<DonationData> donate;

    QString issuesUrl;
    QString sourceUrl;
    QString wikiUrl;
    QString discordUrl;
};

struct IndexedPack {
    QVariant addonId;
    Provider provider;
    QString name;
    QString slug;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;

    bool versionsLoaded = false;
    QVector<IndexedVersion> versions;

    // Don't load by default, since some modplatform don't have that info
    bool extraDataLoaded = true;
    ExtraPackData extraData;
};

}  // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::IndexedPack)
Q_DECLARE_METATYPE(ModPlatform::Provider)
