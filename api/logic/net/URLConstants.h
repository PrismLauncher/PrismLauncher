/* Copyright 2013-2019 MultiMC Contributors
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
const QString AWS_DOWNLOAD_VERSIONS("https://s3.amazonaws.com/Minecraft.Download/versions/");
const QString RESOURCE_BASE("https://resources.download.minecraft.net/");
const QString LIBRARY_BASE("https://libraries.minecraft.net/");
const QString SKINS_BASE("https://crafatar.com/skins/");
const QString AUTH_BASE("https://authserver.mojang.com/");
const QString MOJANG_STATUS_URL("https://status.mojang.com/check");
const QString IMGUR_BASE_URL("https://api.imgur.com/3/");
const QString FMLLIBS_OUR_BASE_URL("https://files.multimc.org/fmllibs/");
const QString FMLLIBS_FORGE_BASE_URL("https://files.minecraftforge.net/fmllibs/");
const QString TRANSLATIONS_BASE_URL("https://files.multimc.org/translations/");
const QString FTB_CDN_BASE_URL("https://ftb.forgecdn.net/FTB2/");

QString getJarPath(QString version);
QString getLegacyJarUrl(QString version);
}
