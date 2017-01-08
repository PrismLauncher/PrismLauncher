/* Copyright 2013-2017 MultiMC Contributors
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
const QString AWS_DOWNLOAD_VERSIONS("s3.amazonaws.com/Minecraft.Download/versions/");
const QString RESOURCE_BASE("resources.download.minecraft.net/");
const QString LIBRARY_BASE("libraries.minecraft.net/");
//const QString SKINS_BASE("skins.minecraft.net/MinecraftSkins/");
const QString SKINS_BASE("crafatar.com/skins/");
const QString AUTH_BASE("authserver.mojang.com/");
const QString FORGE_LEGACY_URL("http://files.minecraftforge.net/minecraftforge/json");
const QString FORGE_GRADLE_URL("http://files.minecraftforge.net/maven/net/minecraftforge/forge/json");
const QString MOJANG_STATUS_URL("http://status.mojang.com/check");
const QString MOJANG_STATUS_NEWS_URL("http://status.mojang.com/news");
const QString LITELOADER_URL("http://dl.liteloader.com/versions/versions.json");
const QString IMGUR_BASE_URL("https://api.imgur.com/3/");
const QString FMLLIBS_OUR_BASE_URL("http://files.multimc.org/fmllibs/");
const QString FMLLIBS_FORGE_BASE_URL("http://files.minecraftforge.net/fmllibs/");
const QString TRANSLATIONS_BASE_URL("http://files.multimc.org/translations/");

QString getJarPath(QString version);
QString getLegacyJarUrl(QString version);
}
