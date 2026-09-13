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

namespace {
Result<> loadIndexedVersion(ATLauncher::IndexedVersion& v, QJsonObject& obj)
{
    TRY_INTO(v.version, Json::requireString(obj, "version"))
    TRY_INTO(v.minecraft, Json::requireString(obj, "minecraft"))
    return {};
}
}  // namespace

Result<> ATLauncher::loadIndexedPack(ATLauncher::IndexedPack& m, QJsonObject& obj)
{
    TRY_INTO(m.id, Json::requireInteger(obj, "id"))
    TRY_INTO(m.position, Json::requireInteger(obj, "position"))
    TRY_INTO(m.name, Json::requireString(obj, "name"))
    auto type = Json::requireString(obj, "type");
    TRY(type)
    m.type = type.value() == "private" ? ATLauncher::PackType::Private : ATLauncher::PackType::Public;
    auto versionsArr = Json::requireArray(obj, "versions");
    TRY(versionsArr)
    for (const auto versionRaw : versionsArr.value()) {
        auto versionObj = Json::requireObject(versionRaw);
        TRY(versionObj)
        ATLauncher::IndexedVersion version;
        TRY(loadIndexedVersion(version, versionObj.value()))
        m.versions.append(version);
    }
    m.system = obj["system"].toBool();
    m.description = obj["description"].toString("");

    static const QRegularExpression s_regex("[^A-Za-z0-9]");
    TRY_INTO(m.safeName,
             Json::requireString(obj, "name").and_then([](auto v) -> Result<QString> { return v.replace(s_regex, "").toLower() + ".png"; }))
    return {};
}
