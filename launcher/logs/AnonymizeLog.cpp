// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
#include "AnonymizeLog.h"

#include <QRegularExpression>

struct RegReplace {
    RegReplace(QRegularExpression r, QString w) : reg(r), with(w) { reg.optimize(); }
    QRegularExpression reg;
    QString with;
};

static const QVector<RegReplace> anonymizeRules = {
    // OS username
    RegReplace(QRegularExpression("C:\\\\Users\\\\([^\\\\]+)\\\\", QRegularExpression::CaseInsensitiveOption),
               "C:\\Users\\********\\"),  // windows
    RegReplace(QRegularExpression("C:\\/Users\\/([^\\/]+)\\/", QRegularExpression::CaseInsensitiveOption),
               "C:/Users/********/"),  // windows with forward slashes
    RegReplace(QRegularExpression("(?<!\\\\w)\\/home\\/[^\\/]+\\/", QRegularExpression::CaseInsensitiveOption),
               "/home/********/"),  // linux
    RegReplace(QRegularExpression("(?<!\\\\w)\\/Users\\/[^\\/]+\\/", QRegularExpression::CaseInsensitiveOption),
               "/Users/********/"),  // macos
    // Tokens
    RegReplace(QRegularExpression("\\(Session ID is [^\\)]+\\)", QRegularExpression::CaseInsensitiveOption),
               "(Session ID is <SESSION_TOKEN>)"),  // SESSION_TOKEN
    RegReplace(QRegularExpression("new refresh token: \"[^\"]+\"", QRegularExpression::CaseInsensitiveOption),
               "new refresh token: \"<TOKEN>\""),  // refresh token
    RegReplace(QRegularExpression("\"device_code\" :  \"[^\"]+\"", QRegularExpression::CaseInsensitiveOption),
               "\"device_code\" :  \"<DEVICE_CODE>\""),  // device code
    // MC username and UUID
    RegReplace(QRegularExpression("Setting user: [a-zA-Z0-9_]{2,16}"), "Setting user: *****"),
    RegReplace(QRegularExpression("--username, [a-zA-Z0-9_]{2,16}"), "--username, *****"),
    RegReplace(QRegularExpression("[a-zA-Z0-9_]{2,16} joined the game"), "***** joined the game"),
    RegReplace(QRegularExpression(R"(Player \[[a-zA-Z0-9_]{2,16}\] joined\.)"), "***** joined the game"),
    RegReplace(QRegularExpression("[a-zA-Z0-9_]{2,16} lost connection: "), "***** lost connection: "),
    RegReplace(QRegularExpression("[a-zA-Z0-9_]{2,16} left the game"), "***** left the game"),
    RegReplace(QRegularExpression(R"([a-zA-Z0-9_]{2,16} has (made the advancement)|(reached the goal) \[[)"), "***** has made the advancement ["),
    RegReplace(QRegularExpression("UUID of player [a-zA-Z0-9_]{2,16} is [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"), "UUID of player ***** is *****"),
    RegReplace(QRegularExpression("id=[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12},name=[a-zA-Z0-9_]{2,16}"), "id=*****,name=*****"),
    RegReplace(QRegularExpression(R"([a-zA-Z0-9_]{2,16}\[[^\]]{3,30}\] logged in with entity id )"), "*****[****] logged in with entity id "),
    RegReplace(QRegularExpression("[a-zA-Z0-9_]{2,16} \\([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\)"), "***** (*****)"),
    RegReplace(QRegularExpression("ServerPlayer\\['[a-zA-Z0-9_]{2,16}'"), "ServerPlayer['*****'"), // crash report
    RegReplace(QRegularExpression("Player: [a-zA-Z0-9_]{2,16}"), "Player: *****"),
    RegReplace(QRegularExpression("player [a-zA-Z0-9_]{2,16}"), "player *****"),
    RegReplace(QRegularExpression("Authenticating to Mojang as [a-zA-Z0-9_]{2,16} \\([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\)"), "Authenticating to Mojang as ***** (*****)"),
};

void anonymizeLog(QString& log)
{
    for (const auto& rule : anonymizeRules) {
        log.replace(rule.reg, rule.with);
    }
}
