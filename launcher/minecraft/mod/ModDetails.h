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
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
*      Copyright 2013-2021 MultiMC Contributors
*
*      Licensed under the Apache License, Version 2.0 (the "License");
*      you may not use this file except in compliance with the License.
*      You may obtain a copy of the License at
*
*          http://www.apache.org/licenses/LICENSE-2.0
*
*      Unless required by applicable law or agreed to in writing, software
*      distributed under the License is distributed on an "AS IS" BASIS,
*      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*      See the License for the specific language governing permissions and
*      limitations under the License.
*/

#pragma once

#include <memory>

#include <QString>
#include <QStringList>

#include "minecraft/mod/MetadataHandler.h"

enum class ModStatus {
    Installed,      // Both JAR and Metadata are present
    NotInstalled,   // Only the Metadata is present
    NoMetadata,     // Only the JAR is present
    Unknown,        // Default status
};

struct ModDetails
{
    /* Mod ID as defined in the ModLoader-specific metadata */
    QString mod_id = {};
    
    /* Human-readable name */
    QString name = {};
    
    /* Human-readable mod version */
    QString version = {};
    
    /* Human-readable minecraft version */
    QString mcversion = {};
    
    /* URL for mod's home page */
    QString homeurl = {};
    
    /* Human-readable description */
    QString description = {};

    /* List of the author's names */
    QStringList authors = {};

    /* Installation status of the mod */
    ModStatus status = ModStatus::Unknown;

    /* Metadata information, if any */
    std::shared_ptr<Metadata::ModStruct> metadata = nullptr;

    ModDetails() = default;

    /** Metadata should be handled manually to properly set the mod status. */
    ModDetails(ModDetails& other)
        : mod_id(other.mod_id)
        , name(other.name)
        , version(other.version)
        , mcversion(other.mcversion)
        , homeurl(other.homeurl)
        , description(other.description)
        , authors(other.authors)
        , status(other.status)
    {}
};
