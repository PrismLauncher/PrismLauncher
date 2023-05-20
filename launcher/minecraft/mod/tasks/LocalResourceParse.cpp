// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include <QObject> 
#include <utility>

#include <memory>

#include "LocalResourceParse.h"

#include "LocalDataPackParseTask.h"
#include "LocalModParseTask.h"
#include "LocalResourcePackParseTask.h"
#include "LocalShaderPackParseTask.h"
#include "LocalTexturePackParseTask.h"
#include "LocalWorldSaveParseTask.h"


static const QMap<PackedResourceType, QString> s_packed_type_names = {
    {PackedResourceType::ResourcePack, QObject::tr("resource pack")},
    {PackedResourceType::TexturePack,  QObject::tr("texture pack")},
    {PackedResourceType::DataPack, QObject::tr("data pack")},
    {PackedResourceType::ShaderPack, QObject::tr("shader pack")},
    {PackedResourceType::WorldSave, QObject::tr("world save")},
    {PackedResourceType::Mod , QObject::tr("mod")},
    {PackedResourceType::UNKNOWN, QObject::tr("unknown")}
};

static const QMap<ResourceManagmentType, QString> s_managment_type_names = {
    {ResourceManagmentType::PackManaged, "PackManaged"},
    {ResourceManagmentType::UserInstalled, "UserInstalled"},
    {ResourceManagmentType::External, "External"}
};

namespace ResourceUtils {
std::pair<PackedResourceType, Resource::Ptr> identify(QFileInfo file){
    if (file.exists() && file.isFile()) {
        if (auto mod = ModUtils::validate(file); mod) {
            qDebug() << file.fileName() << "is a mod";
            return std::make_pair(PackedResourceType::Mod, mod);
        } else if (auto rp = ResourcePackUtils::validate(file); rp) {
            qDebug() << file.fileName() << "is a resource pack";
            return std::make_pair(PackedResourceType::ResourcePack, rp);
        } else if (auto tp = TexturePackUtils::validate(file); tp) {
            qDebug() << file.fileName() << "is a pre 1.6 texture pack";
            return std::make_pair(PackedResourceType::TexturePack, tp);
        } else if (auto dp = DataPackUtils::validate(file); dp) {
            qDebug() << file.fileName() << "is a data pack";
            return std::make_pair(PackedResourceType::DataPack, dp);
        } else if (auto world = WorldSaveUtils::validate(file); world) {
            qDebug() << file.fileName() << "is a world save";
            return std::make_pair(PackedResourceType::WorldSave, world);
        } else if (auto sp = ShaderPackUtils::validate(file); sp) {
            qDebug() << file.fileName() << "is a shader pack";
            return std::make_pair(PackedResourceType::ShaderPack, sp);
        } else {
            qDebug() << "Can't Identify" << file.fileName() ;
            return std::make_pair(PackedResourceType::UNKNOWN, makeShared<Resource>(file));
        }
    } else {
        qDebug() << "Can't find" << file.absolutePath();
    }
    return std::make_pair(PackedResourceType::INVALID, nullptr);
}

QString getPackedTypeName(PackedResourceType type) {
    return s_packed_type_names.constFind(type).value();
}

QString getManagmentTypeName(ResourceManagmentType type) {
    return s_managment_type_names.constFind(type).value();
}

}
