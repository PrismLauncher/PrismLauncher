// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "ParseUtils.h"
#include "minecraft/Library.h"
#include "minecraft/PackProfile.h"
#include "minecraft/VersionFile.h"

#include <Version.h>

static bool isMinecraftVersion(const QString& uid)
{
    return uid == "net.minecraft";
}

void VersionFile::applyTo(LaunchProfile* profile, const RuntimeContext& runtimeContext)
{
    // Only real Minecraft can set those. Don't let anything override them.
    if (isMinecraftVersion(uid)) {
        profile->applyMinecraftVersion(version);
        profile->applyMinecraftVersionType(type);
        // HACK: ignore assets from other version files than Minecraft
        // workaround for stupid assets issue caused by amazon:
        // https://www.theregister.co.uk/2017/02/28/aws_is_awol_as_s3_goes_haywire/
        profile->applyMinecraftAssets(mojangAssetIndex);
    }

    profile->applyMainJar(mainJar);
    profile->applyMainClass(mainClass);
    profile->applyAppletClass(appletClass);
    profile->applyMinecraftArguments(minecraftArguments);
    profile->applyAddnJvmArguments(addnJvmArguments);
    profile->applyTweakers(addTweakers);
    profile->applyJarMods(jarMods);
    profile->applyMods(mods);
    profile->applyTraits(traits);
    profile->applyCompatibleJavaMajors(compatibleJavaMajors);
    profile->applyCompatibleJavaName(compatibleJavaName);

    for (auto library : libraries) {
        profile->applyLibrary(library, runtimeContext);
    }
    for (auto mavenFile : mavenFiles) {
        profile->applyMavenFile(mavenFile, runtimeContext);
    }
    for (auto agent : agents) {
        profile->applyAgent(agent, runtimeContext);
    }
    profile->applyProblemSeverity(getProblemSeverity());
}
