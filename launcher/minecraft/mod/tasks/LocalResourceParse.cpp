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

#include "LocalResourceParse.h"

#include "LocalDataPackParseTask.h"
#include "LocalModParseTask.h"
#include "LocalResourcePackParseTask.h"
#include "LocalShaderPackParseTask.h"
#include "LocalTexturePackParseTask.h"
#include "LocalWorldSaveParseTask.h"

static const QMap<PackedResourceType, QString> s_packed_type_names = { { PackedResourceType::ResourcePack, QObject::tr("resource pack") },
                                                                       { PackedResourceType::TexturePack, QObject::tr("texture pack") },
                                                                       { PackedResourceType::DataPack, QObject::tr("data pack") },
                                                                       { PackedResourceType::ShaderPack, QObject::tr("shader pack") },
                                                                       { PackedResourceType::WorldSave, QObject::tr("world save") },
                                                                       { PackedResourceType::Mod, QObject::tr("mod") },
                                                                       { PackedResourceType::UNKNOWN, QObject::tr("unknown") } };

namespace ResourceUtils {
PackedResourceType identify(QFileInfo file)
{
    if (file.exists() && file.isFile()) {
        if (ModUtils::validate(file)) {
            // mods can contain resource and data packs so they must be tested first
            qDebug() << file.fileName() << "is a mod";
            return PackedResourceType::Mod;
        } else if (ResourcePackUtils::validate(file)) {
            qDebug() << file.fileName() << "is a resource pack";
            return PackedResourceType::ResourcePack;
        } else if (TexturePackUtils::validate(file)) {
            qDebug() << file.fileName() << "is a pre 1.6 texture pack";
            return PackedResourceType::TexturePack;
        } else if (DataPackUtils::validate(file)) {
            qDebug() << file.fileName() << "is a data pack";
            return PackedResourceType::DataPack;
        } else if (WorldSaveUtils::validate(file)) {
            qDebug() << file.fileName() << "is a world save";
            return PackedResourceType::WorldSave;
        } else if (ShaderPackUtils::validate(file)) {
            qDebug() << file.fileName() << "is a shader pack";
            return PackedResourceType::ShaderPack;
        } else {
            qDebug() << "Can't Identify" << file.fileName();
        }
    } else {
        qDebug() << "Can't find" << file.absolutePath();
    }
    return PackedResourceType::UNKNOWN;
}

QString getPackedTypeName(PackedResourceType type)
{
    return s_packed_type_names.constFind(type).value();
}

}  // namespace ResourceUtils
