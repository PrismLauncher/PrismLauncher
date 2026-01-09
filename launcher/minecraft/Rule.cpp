// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2025 TheKodeToad <TheKodeToad@proton.me>
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
#include <QJsonObject>

#include "Rule.h"

Rule Rule::fromJson(const QJsonObject& object)
{
    Rule result;

    if (object["action"] == "allow")
        result.m_action = Allow;
    else if (object["action"] == "disallow")
        result.m_action = Disallow;

    if (auto os = object["os"]; os.isObject()) {
        if (auto name = os["name"].toString(); !name.isNull()) {
            result.m_os = OS{
                name,
                os["version"].toString(),
            };
        }
    }

    return result;
}

QJsonObject Rule::toJson()
{
    QJsonObject result;

    if (m_action == Allow)
        result["action"] = "allow";
    else if (m_action == Disallow)
        result["action"] = "disallow";

    if (m_os.has_value()) {
        QJsonObject os;

        os["name"] = m_os->name;

        if (!m_os->version.isEmpty())
            os["version"] = m_os->version;

        result["os"] = os;
    }

    return result;
}

Rule::Action Rule::apply(const RuntimeContext& runtimeContext)
{
    if (m_os.has_value() && !runtimeContext.classifierMatches(m_os->name))
        return Defer;

    return m_action;
}
