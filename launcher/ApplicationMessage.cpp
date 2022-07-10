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

#include "ApplicationMessage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include "Json.h"

void ApplicationMessage::parse(const QByteArray & input) {
    auto doc = Json::requireDocument(input, "ApplicationMessage");
    auto root = Json::requireObject(doc, "ApplicationMessage");

    command = root.value("command").toString();
    args.clear();

    auto parsedArgs = root.value("args").toObject();
    for(auto iter = parsedArgs.begin(); iter != parsedArgs.end(); iter++) {
        args[iter.key()] = iter.value().toString();
    }
}

QByteArray ApplicationMessage::serialize() {
    QJsonObject root;
    root.insert("command", command);
    QJsonObject outArgs;
    for (auto iter = args.begin(); iter != args.end(); iter++) {
        outArgs[iter.key()] = iter.value();
    }
    root.insert("args", outArgs);

    return Json::toText(root);
}
