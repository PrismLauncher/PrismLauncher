#pragma once

#include "modplatform/ModIndex.h"

#include <QString>
#include <QUrl>
#include <QVariant>

class QDir;

// Mod from launcher/minecraft/mod/Mod.h
class Mod;

namespace Packwiz {

class V1 {
   public:
    struct Mod {
        QString name {};
        QString filename {};
        // FIXME: make side an enum
        QString side {"both"};

        // [download]
        QUrl url {};
        // FIXME: make hash-format an enum
        QString hash_format {};
        QString hash {};

        // [update]
        ModPlatform::Provider provider {};
        QVariant file_id {};
        QVariant project_id {};

       public:
        // This is a totally heuristic, but should work for now.
        auto isValid() const -> bool { return !name.isEmpty() && !project_id.isNull(); }

        // Different providers can use different names for the same thing
        // Modrinth-specific
        auto mod_id() -> QVariant& { return project_id; }
        auto version() -> QVariant& { return file_id; }
    };

    /* Generates the object representing the information in a mod.toml file via
     * its common representation in the launcher, when downloading mods.
     * */
    static auto createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod;
    /* Generates the object representing the information in a mod.toml file via
     * its common representation in the launcher.
     * */
    static auto createModFormat(QDir& index_dir, ::Mod& internal_mod) -> Mod;

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
    static auto getIndexForMod(QDir& index_dir, QString& index_file_name) -> Mod;
};

} // namespace Packwiz
