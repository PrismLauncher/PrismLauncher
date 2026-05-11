// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "modplatform/ModIndex.h"

#include <QString>
#include <QUrl>
#include <QVariant>

class QDir;

namespace Packwiz {

class V1 {
   public:
    // can also represent other resources beside loader mods - but this is what packwiz calls it
    struct Mod {
        QString slug{};
        QString name{};
        QString filename{};
        ModPlatform::SideType side{ ModPlatform::SideType::UniversalSide };
        ModPlatform::ModLoaderTypes loaders;
        QStringList mcVersions;
        ModPlatform::IndexedVersionType releaseType;

        // [download]
        QString mode{};
        QUrl url{};
        QString hashFormat{};
        QString hash{};

        // [update]
        ModPlatform::ResourceProvider provider{};
        QVariant fileId{};
        QVariant projectId{};
        QString versionNumber{};

        QList<ModPlatform::Dependency> dependencies;
        bool lockUpdate;

       public:
        // This is a totally heuristic, but should work for now.
        auto isValid() const -> bool { return !slug.isEmpty() && !projectId.isNull(); }

        // Different providers can use different names for the same thing
        // Modrinth-specific
        auto modId() -> QVariant& { return projectId; }
        auto version() -> QVariant& { return fileId; }
    };

    /* Generates the object representing the information in a mod.pw.toml file via
     * its common representation in the launcher, when downloading mods.
     * */
    static auto createModFormat(const QDir& indexDir, ModPlatform::IndexedPack& modPack, ModPlatform::IndexedVersion& modVersion) -> Mod;

    /* Updates the mod index for the provided mod.
     * This creates a new index if one does not exist already
     * TODO: Ask the user if they want to override, and delete the old mod's files, or keep the old one.
     * */
    static void updateModIndex(const QDir& indexDir, Mod& mod);

    /* Deletes the metadata for the mod with the given slug. If the metadata doesn't exist, it does nothing. */
    static void deleteModIndex(const QDir& indexDir, QString& modSlug);

    /* Gets the metadata for a mod with a particular file name.
     * If the mod doesn't have a metadata, it simply returns an empty Mod object.
     * */
    static auto getIndexForMod(const QDir& indexDir, const QString& slug) -> Mod;

    /* Gets the metadata for a mod with a particular id.
     * If the mod doesn't have a metadata, it simply returns an empty Mod object.
     * */
    static auto getIndexForMod(const QDir& indexDir, QVariant& modId) -> Mod;
};

}  // namespace Packwiz
