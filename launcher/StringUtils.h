// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
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

#include <QPair>
#include <QString>
#include <QUrl>
#include <utility>

namespace StringUtils {

#if defined Q_OS_WIN32
using string = std::wstring;

inline string toStdString(QString s)
{
    return s.toStdWString();
}
inline QString fromStdString(string s)
{
    return QString::fromStdWString(s);
}
#else
using string = std::string;

inline string toStdString(QString s)
{
    return s.toStdString();
}
inline QString fromStdString(string s)
{
    return QString::fromStdString(s);
}
#endif

int naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs);

/**
 * @brief Truncate a url while keeping its readability py placing the `...` in the middle of the path
 * @param url Url to truncate
 * @param max_len max length of url in characters
 * @param hard_limit if truncating the path can't get the url short enough, truncate it normally.
 */
QString truncateUrlHumanFriendly(QUrl& url, int max_len, bool hard_limit = false);

QString humanReadableFileSize(double bytes, bool use_si = false, int decimal_points = 1);

QString getRandomAlphaNumeric();

QPair<QString, QString> splitFirst(const QString& s, const QString& sep, Qt::CaseSensitivity cs = Qt::CaseSensitive);
QPair<QString, QString> splitFirst(const QString& s, QChar sep, Qt::CaseSensitivity cs = Qt::CaseSensitive);
QPair<QString, QString> splitFirst(const QString& s, const QRegularExpression& re);

QString htmlListPatch(QString htmlStr);

}  // namespace StringUtils
