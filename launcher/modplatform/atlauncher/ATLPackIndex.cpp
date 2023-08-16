/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2021 Petr Mrazek <peterix@gmail.com>
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

#include "ATLPackIndex.h"

#include <QRegularExpression>

#include "Json.h"

static void loadIndexedVersion(ATLauncher::IndexedVersion& v, QJsonObject& obj)
{
    v.version = Json::requireString(obj, "version");
    v.minecraft = Json::requireString(obj, "minecraft");
}

void ATLauncher::loadIndexedPack(ATLauncher::IndexedPack& m, QJsonObject& obj)
{
    m.id = Json::requireInteger(obj, "id");
    m.position = Json::requireInteger(obj, "position");
    m.name = Json::requireString(obj, "name");
    m.type = Json::requireString(obj, "type") == "private" ? ATLauncher::PackType::Private : ATLauncher::PackType::Public;
    auto versionsArr = Json::requireArray(obj, "versions");
    for (const auto versionRaw : versionsArr) {
        auto versionObj = Json::requireObject(versionRaw);
        ATLauncher::IndexedVersion version;
        loadIndexedVersion(version, versionObj);
        m.versions.append(version);
    }
    m.system = Json::ensureBoolean(obj, QString("system"), false);
    m.description = Json::ensureString(obj, "description", "");

    m.safeName = Json::requireString(obj, "name").replace(QRegularExpression("[^A-Za-z0-9]"), "");
}
