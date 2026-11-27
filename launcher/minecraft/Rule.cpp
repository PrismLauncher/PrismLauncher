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

Rule Rule::fromJson(const QJsonObject& obj)
{
    auto action = Action::Defer;
    if (obj["action"] == "allow") {
        action = Action::Allow;
    } else if (obj["action"] == "disallow") {
        action = Action::Disallow;
    }

    std::optional<OS> os;
    if (const auto osJSON = obj["os"]; osJSON.isObject()) {
        if (auto name = osJSON["name"].toString(); !name.isNull()) {
            os = OS{
                .name = std::move(name),
                .version = osJSON["version"].toString(),
            };
        }
    }

    QList<std::pair<QString, bool>> features;

    const auto featuresObj = obj["features"].toObject();
    features.reserve(featuresObj.size());
    for (const auto& [key, value] : featuresObj.asKeyValueRange()) {
        if (value.isBool()) {
            features.append(std::pair{ key.toString(), value.toBool() });
        }
    }

    return Rule{
        .action = action,
        .os = std::move(os),
        .features = std::move(features),
    };
}

QJsonObject Rule::toJson() const
{
    QJsonObject result;

    if (action == Action::Allow) {
        result["action"] = "allow";
    } else if (action == Action::Disallow) {
        result["action"] = "disallow";
    }

    if (os.has_value()) {
        QJsonObject osObj;

        osObj["name"] = os->name;

        if (!os->version.isEmpty()) {
            osObj["version"] = os->version;
        }

        result["os"] = std::move(osObj);
    }

    QJsonObject featuresObj;
    for (const auto& [feature, shouldHaveFeature] : features) {
        featuresObj[feature] = shouldHaveFeature;
    }
    if (!featuresObj.isEmpty()) {
        result["features"] = std::move(featuresObj);
    }

    return result;
}

Rule::Action Rule::applyRuleset(const QList<Rule>& rules, const RuleContext& ctx)
{
    if (rules.isEmpty()) {
        return Action::Allow;
    }

    auto result = Action::Disallow;
    for (const auto& rule : rules) {
        if (auto choice = rule.apply(ctx); choice != Action::Defer) {
            result = choice;
        }
    }
    return result;
}

Rule::Action Rule::apply(const RuleContext& ctx) const
{
    if (os.has_value() && (ctx.runtime == nullptr || !ctx.runtime->classifierMatches(os->name))) {
        return Action::Defer;
    }

    for (const auto& [feature, shouldHaveFeature] : features) {
        const bool hasFeature = ctx.features != nullptr && ctx.features->contains(feature);
        qDebug() << "feature =" << feature << "shouldHaveFeature = " << shouldHaveFeature << "hasFeature" << hasFeature;
        if (hasFeature != shouldHaveFeature) {
            return Action::Defer;
        }
    }

    return action;
}

RuleGuardedStrings RuleGuardedStrings::fromJson(const QJsonValue& json)
{
    if (json.isObject()) {
        const auto valueJSON = json["value"];
        QStringList value;
        if (valueJSON.isString()) {
            value.append(valueJSON.toString());
        } else if (valueJSON.isArray()) {
            for (const auto& item : valueJSON.toArray()) {
                if (item.isString()) {
                    value.append(item.toString());
                }
            }
        }

        QList<Rule> rules;
        for (const auto& rule : json["rules"].toArray()) {
            rules.append(Rule::fromJson(rule.toObject()));
        }

        return {
            .value = std::move(value),
            .rules = std::move(rules),
        };
    }

    return {
        .value = { json.toString() },
        .rules = {},
    };
}

QJsonValue RuleGuardedStrings::toJson() const
{
    if (rules.isEmpty() && value.size() == 1) {
        return value[0];
    }

    QJsonArray valueJSON;
    for (const auto& item : value) {
        valueJSON.append(item);
    }

    QJsonArray rulesJSON;
    for (const auto& rule : rules) {
        rulesJSON.append(rule.toJson());
    }

    return QJsonObject{
        { "value", valueJSON },
        { "rules", rulesJSON },
    };
}
