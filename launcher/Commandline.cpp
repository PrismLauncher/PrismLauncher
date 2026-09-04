// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
 *      Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "Commandline.h"

/**
 * @file libutil/src/cmdutils.cpp
 */

namespace Commandline {

// commandline splitter
QStringList splitArgs(const QString& args)
{
    QStringList argv;
    QString current;
    bool escape = false;
    QChar inquotes;
    for (int i = 0; i < args.length(); i++) {
        QChar cchar = args.at(i);

        // \ escaped
        if (escape) {
            current += cchar;
            escape = false;
            // in "quotes"
        } else if (!inquotes.isNull()) {
            if (cchar == '\\') {
                escape = true;
            } else if (cchar == inquotes) {
                inquotes = QChar::Null;
            } else {
                current += cchar;
            }
            // otherwise
        } else {
            if (cchar == ' ') {
                if (!current.isEmpty()) {
                    argv << current;
                    current.clear();
                }
            } else if (cchar == '"' || cchar == '\'') {
                inquotes = cchar;
            } else {
                current += cchar;
            }
        }
    }
    if (!current.isEmpty()) {
        argv << current;
    }
    return argv;
}

QString expandVariables(const QString& input, const QProcessEnvironment& dict)
{
    QString result = input;

    enum State : std::uint8_t { Base, MaybeBrace, Variable, Brace } state = Base;
    int startIdx = -1;
    for (int i = 0; i < result.length();) {
        QChar c = result.at(i++);
        switch (state) {
            case Base:
                if (c == '$') {
                    state = MaybeBrace;
                }
                break;
            case MaybeBrace:
                if (c == '{') {
                    state = Brace;
                    startIdx = i;
                } else if (c.isLetterOrNumber() || c == '_') {
                    state = Variable;
                    startIdx = i - 1;
                } else {
                    state = Base;
                }
                break;
            case Brace:
                if (c == '}') {
                    const auto res = dict.value(result.mid(startIdx, i - 1 - startIdx), "");
                    if (!res.isEmpty()) {
                        result.replace(startIdx - 2, i - startIdx + 2, res);
                        i = startIdx - 2 + res.length();
                    }
                    state = Base;
                }
                break;
            case Variable:
                if (!c.isLetterOrNumber() && c != '_') {
                    const auto res = dict.value(result.mid(startIdx, i - startIdx - 1), "");
                    if (!res.isEmpty()) {
                        result.replace(startIdx - 1, i - startIdx, res);
                        i = startIdx - 1 + res.length();
                    }
                    state = Base;
                }
                break;
        }
    }
    if (state == Variable) {
        if (const auto res = dict.value(result.mid(startIdx), ""); !res.isEmpty()) {
            result.replace(startIdx - 1, result.length() - startIdx + 1, res);
        }
    }
    return result;
}

QString quoteForSplitCommand(const QString& input)
{
    if (!input.contains(' ')) {
        return input;
    }

    QString escaped = input;
    escaped.replace("\"", "\"\"\"");
    return "\"" + escaped + "\"";
}
}  // namespace Commandline
