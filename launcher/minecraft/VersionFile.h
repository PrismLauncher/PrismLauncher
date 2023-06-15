// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/Rule.h"
#include "ProblemProvider.h"
#include "Library.h"
#include "Agent.h"
#include <meta/JsonFormat.h>

class PackProfile;
class VersionFile;
class LaunchProfile;
struct MojangDownloadInfo;
struct MojangAssetIndexInfo;

using VersionFilePtr = std::shared_ptr<VersionFile>;
class VersionFile : public ProblemContainer
{
    friend class MojangVersionFormat;
    friend class OneSixVersionFormat;
public: /* methods */
    void applyTo(LaunchProfile* profile, const RuntimeContext & runtimeContext);

public: /* data */
    /// Prism Launcher: order hint for this version file if no explicit order is set
    int order = 0;

    /// Prism Launcher: human readable name of this package
    QString name;

    /// Prism Launcher: package ID of this package
    QString uid;

    /// Prism Launcher: version of this package
    QString version;

    /// Prism Launcher: DEPRECATED dependency on a Minecraft version
    QString dependsOnMinecraftVersion;

    /// Mojang: DEPRECATED used to version the Mojang version format
    int minimumLauncherVersion = -1;

    /// Mojang: DEPRECATED version of Minecraft this is
    QString minecraftVersion;

    /// Mojang: class to launch Minecraft with
    QString mainClass;

    /// Prism Launcher: class to launch legacy Minecraft with (embed in a custom window)
    QString appletClass;

    /// Mojang: Minecraft launch arguments (may contain placeholders for variable substitution)
    QString minecraftArguments;

    /// Prism Launcher: Additional JVM launch arguments
    QStringList addnJvmArguments;

    /// Mojang: list of compatible java majors
    QList<int> compatibleJavaMajors;

    /// Mojang: type of the Minecraft version
    QString type;

    /// Mojang: the time this version was actually released by Mojang
    QDateTime releaseTime;

    /// Mojang: DEPRECATED the time this version was last updated by Mojang
    QDateTime updateTime;

    /// Mojang: DEPRECATED asset group to be used with Minecraft
    QString assets;

    /// Prism Launcher: list of tweaker mod arguments for launchwrapper
    QStringList addTweakers;

    /// Mojang: list of libraries to add to the version
    QList<LibraryPtr> libraries;

    /// Prism Launcher: list of maven files to put in the libraries folder, but not in classpath
    QList<LibraryPtr> mavenFiles;

    /// Prism Launcher: list of agents to add to JVM arguments
    QList<AgentPtr> agents;

    /// The main jar (Minecraft version library, normally)
    LibraryPtr mainJar;

    /// Prism Launcher: list of attached traits of this version file - used to enable features
    QSet<QString> traits;

    /// Prism Launcher: list of jar mods added to this version
    QList<LibraryPtr> jarMods;

    /// Prism Launcher: list of mods added to this version
    QList<LibraryPtr> mods;

    /**
     * Prism Launcher: set of packages this depends on
     * NOTE: this is shared with the meta format!!!
     */
    Meta::RequireSet m_requires;

    /**
     * Prism Launcher: set of packages this conflicts with
     * NOTE: this is shared with the meta format!!!
     */
    Meta::RequireSet conflicts;

    /// is volatile -- may be removed as soon as it is no longer needed by something else
    bool m_volatile = false;

public:
    // Mojang: DEPRECATED list of 'downloads' - client jar, server jar, windows server exe, maybe more.
    QMap <QString, std::shared_ptr<MojangDownloadInfo>> mojangDownloads;

    // Mojang: extended asset index download information
    std::shared_ptr<MojangAssetIndexInfo> mojangAssetIndex;
};

