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

#include <QJsonObject>
#include <QJsonArray>

#include "Rule.h"

RuleAction RuleAction_fromString(QString name)
{
    if (name == "allow")
        return Allow;
    if (name == "disallow")
        return Disallow;
    return Defer;
}

QList<std::shared_ptr<Rule>> rulesFromJsonV4(const QJsonObject &objectWithRules)
{
    QList<std::shared_ptr<Rule>> rules;
    auto rulesVal = objectWithRules.value("rules");
    if (!rulesVal.isArray())
        return rules;

    QJsonArray ruleList = rulesVal.toArray();
    for (auto ruleVal : ruleList)
    {
        std::shared_ptr<Rule> rule;
        if (!ruleVal.isObject())
            continue;
        auto ruleObj = ruleVal.toObject();
        auto actionVal = ruleObj.value("action");
        if (!actionVal.isString())
            continue;
        auto action = RuleAction_fromString(actionVal.toString());
        if (action == Defer)
            continue;

        auto osVal = ruleObj.value("os");
        if (!osVal.isObject())
        {
            // add a new implicit action rule
            rules.append(ImplicitRule::create(action));
            continue;
        }

        auto osObj = osVal.toObject();
        auto osNameVal = osObj.value("name");
        if (!osNameVal.isString())
            continue;
        QString osName = osNameVal.toString();
        QString versionRegex = osObj.value("version").toString();
        // add a new OS rule
        rules.append(OsRule::create(action, osName, versionRegex));
    }
    return rules;
}

QJsonObject ImplicitRule::toJson()
{
    QJsonObject ruleObj;
    ruleObj.insert("action", m_result == Allow ? QString("allow") : QString("disallow"));
    return ruleObj;
}

QJsonObject OsRule::toJson()
{
    QJsonObject ruleObj;
    ruleObj.insert("action", m_result == Allow ? QString("allow") : QString("disallow"));
    QJsonObject osObj;
    {
        osObj.insert("name", m_system);
        if(!m_version_regexp.isEmpty())
        {
            osObj.insert("version", m_version_regexp);
        }
    }
    ruleObj.insert("os", osObj);
    return ruleObj;
}

