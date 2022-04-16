#pragma once

#include <memory>

#include "modplatform/packwiz/Packwiz.h"

// launcher/minecraft/mod/Mod.h
class Mod;

/* Abstraction file for easily changing the way metadata is stored / handled
 * Needs to be a class because of -Wunused-function and no C++17 [[maybe_unused]]
 * */
class Metadata {
   public:
    using ModStruct = Packwiz::V1::Mod;

    static auto create(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> ModStruct
    {
        return Packwiz::V1::createModFormat(index_dir, mod_pack, mod_version);
    }

    static auto create(QDir& index_dir, Mod& internal_mod) -> ModStruct
    {
        return Packwiz::V1::createModFormat(index_dir, internal_mod);
    }

    static void update(QDir& index_dir, ModStruct& mod)
    {
        Packwiz::V1::updateModIndex(index_dir, mod);
    }

    static void remove(QDir& index_dir, QString& mod_name)
    {
        Packwiz::V1::deleteModIndex(index_dir, mod_name);
    }

    static auto get(QDir& index_dir, QString& mod_name) -> ModStruct
    {
        return Packwiz::V1::getIndexForMod(index_dir, mod_name);
    }
};
