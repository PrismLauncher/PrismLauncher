#pragma once

#include "modplatform/ModIndex.h"

#include <QString>
#include <QUrl>
#include <QVariant>

namespace ModPlatform {
}  // namespace ModPlatform

class QDir;

class Packwiz {
   public:
    struct Mod {
        QString name;
        QString filename;
        // FIXME: make side an enum
        QString side = "both";

        // [download]
        QUrl url;
        // FIXME: make hash-format an enum
        QString hash_format;
        QString hash;

        // [update]
        ModPlatform::Provider provider;
        QVariant file_id;
        QVariant project_id;
    };

    /* Generates the object representing the information in a mod.toml file via its common representation in the launcher */
    static auto createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod;

    /* Updates the mod index for the provided mod.
     * This creates a new index if one does not exist already
     * TODO: Ask the user if they want to override, and delete the old mod's files, or keep the old one.
     * */
    static void updateModIndex(QDir& index_dir, Mod& mod);

    /* Deletes the metadata for the mod with the given name. If the metadata doesn't exist, it does nothing. */
    static void deleteModIndex(QDir& index_dir, QString& mod_name);

    /* Gets the metadata for a mod with a particular name.
     * If the mod doesn't have a metadata, it simply returns an empty Mod object.
     * */
    static auto getIndexForMod(QDir& index_dir, QString& mod_name) -> Mod;
};
