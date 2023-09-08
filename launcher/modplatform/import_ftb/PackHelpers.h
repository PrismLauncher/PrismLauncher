// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include "modplatform/ResourceAPI.h"

namespace FTBImportAPP {

struct Modpack {
    QString path;

    // json data
    QString uuid;
    int id;
    int versionId;
    QString name;
    QString version;
    QString mcVersion;
    // not needed for instance creation
    QVariant jvmArgs;

    std::optional<ModPlatform::ModLoaderType> loaderType;
    QString loaderVersion;

    QIcon icon;
};

typedef QList<Modpack> ModpackList;

Modpack parseDirectory(QString path);

}  // namespace FTBImportAPP

// We need it for the proxy model
Q_DECLARE_METATYPE(FTBImportAPP::Modpack)
