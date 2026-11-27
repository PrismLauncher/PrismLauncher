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

#pragma once

#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>

#include "RuntimeContext.h"

class Library;

struct RuleContext {
    const RuntimeContext* runtime = nullptr;
    const QSet<QString>* features = nullptr;
};

struct Rule {
    enum class Action : std::uint8_t { Allow, Disallow, Defer };

    static Rule fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;

    static Action applyRuleset(const QList<Rule>& rules, const RuleContext& ctx);

    Action apply(const RuleContext& ctx) const;

    struct OS {
        QString name;
        // FIXME: unsupported
        // retained to avoid information being lost from files
        QString version;
    };

    Action action = Action::Defer;
    std::optional<OS> os;
    QList<std::pair<QString, bool>> features;
};

struct RuleGuardedStrings {
    QStringList value;
    QList<Rule> rules;

    static RuleGuardedStrings fromJson(const QJsonValue& json);
    QJsonValue toJson() const;
};
