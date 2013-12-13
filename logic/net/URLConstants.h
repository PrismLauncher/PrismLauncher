/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QString>

namespace URLConstants
{
const QString AWS_DOWNLOAD_BASE("s3.amazonaws.com/Minecraft.Download/");
const QString AWS_DOWNLOAD_VERSIONS(AWS_DOWNLOAD_BASE + "versions/");
const QString AWS_DOWNLOAD_LIBRARIES(AWS_DOWNLOAD_BASE + "libraries/");
const QString AWS_DOWNLOAD_INDEXES(AWS_DOWNLOAD_BASE + "indexes/");
const QString ASSETS_BASE("assets.minecraft.net/");
//const QString MCN_BASE("sonicrules.org/mcnweb.py");
const QString RESOURCE_BASE("resources.download.minecraft.net/");
const QString LIBRARY_BASE("libraries.minecraft.net/");
const QString SKINS_BASE("skins.minecraft.net/MinecraftSkins/");
const QString AUTH_BASE("authserver.mojang.com/");
}
