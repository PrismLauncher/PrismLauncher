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

#include "StringUtils.h"
#include <qpair.h>

#include <QRegularExpression>
#include <QUuid>
#include <cmath>

/// If you're wondering where these came from exactly, then know you're not the only one =D

/// TAKEN FROM Qt, because it doesn't expose it intelligently
static inline QChar getNextChar(const QString& s, int location)
{
    return (location < s.length()) ? s.at(location) : QChar();
}

/// TAKEN FROM Qt, because it doesn't expose it intelligently
int StringUtils::naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs)
{
    int l1 = 0, l2 = 0;
    while (l1 <= s1.count() && l2 <= s2.count()) {
        // skip spaces, tabs and 0's
        QChar c1 = getNextChar(s1, l1);
        while (c1.isSpace())
            c1 = getNextChar(s1, ++l1);

        QChar c2 = getNextChar(s2, l2);
        while (c2.isSpace())
            c2 = getNextChar(s2, ++l2);

        if (c1.isDigit() && c2.isDigit()) {
            while (c1.digitValue() == 0)
                c1 = getNextChar(s1, ++l1);
            while (c2.digitValue() == 0)
                c2 = getNextChar(s2, ++l2);

            int lookAheadLocation1 = l1;
            int lookAheadLocation2 = l2;
            int currentReturnValue = 0;
            // find the last digit, setting currentReturnValue as we go if it isn't equal
            for (QChar lookAhead1 = c1, lookAhead2 = c2; (lookAheadLocation1 <= s1.length() && lookAheadLocation2 <= s2.length());
                 lookAhead1 = getNextChar(s1, ++lookAheadLocation1), lookAhead2 = getNextChar(s2, ++lookAheadLocation2)) {
                bool is1ADigit = !lookAhead1.isNull() && lookAhead1.isDigit();
                bool is2ADigit = !lookAhead2.isNull() && lookAhead2.isDigit();
                if (!is1ADigit && !is2ADigit)
                    break;
                if (!is1ADigit)
                    return -1;
                if (!is2ADigit)
                    return 1;
                if (currentReturnValue == 0) {
                    if (lookAhead1 < lookAhead2) {
                        currentReturnValue = -1;
                    } else if (lookAhead1 > lookAhead2) {
                        currentReturnValue = 1;
                    }
                }
            }
            if (currentReturnValue != 0)
                return currentReturnValue;
        }

        if (cs == Qt::CaseInsensitive) {
            if (!c1.isLower())
                c1 = c1.toLower();
            if (!c2.isLower())
                c2 = c2.toLower();
        }

        int r = QString::localeAwareCompare(c1, c2);
        if (r < 0)
            return -1;
        if (r > 0)
            return 1;

        l1 += 1;
        l2 += 1;
    }

    // The two strings are the same (02 == 2) so fall back to the normal sort
    return QString::compare(s1, s2, cs);
}

QString StringUtils::truncateUrlHumanFriendly(QUrl& url, int max_len, bool hard_limit)
{
    auto display_options = QUrl::RemoveUserInfo | QUrl::RemoveFragment | QUrl::NormalizePathSegments;
    auto str_url = url.toDisplayString(display_options);

    if (str_url.length() <= max_len)
        return str_url;

    auto url_path_parts = url.path().split('/');
    QString last_path_segment = url_path_parts.takeLast();

    if (url_path_parts.size() >= 1 && url_path_parts.first().isEmpty())
        url_path_parts.removeFirst();  // drop empty first segment (from leading / )

    if (url_path_parts.size() >= 1)
        url_path_parts.removeLast();  // drop the next to last path segment

    auto url_template = QStringLiteral("%1://%2/%3%4");

    auto url_compact = url_path_parts.isEmpty()
                           ? url_template.arg(url.scheme(), url.host(), QStringList({ "...", last_path_segment }).join('/'), url.query())
                           : url_template.arg(url.scheme(), url.host(),
                                              QStringList({ url_path_parts.join('/'), "...", last_path_segment }).join('/'), url.query());

    // remove url parts one by one if it's still too long
    while (url_compact.length() > max_len && url_path_parts.size() >= 1) {
        url_path_parts.removeLast();  // drop the next to last path segment
        url_compact = url_path_parts.isEmpty()
                          ? url_template.arg(url.scheme(), url.host(), QStringList({ "...", last_path_segment }).join('/'), url.query())
                          : url_template.arg(url.scheme(), url.host(),
                                             QStringList({ url_path_parts.join('/'), "...", last_path_segment }).join('/'), url.query());
    }

    if ((url_compact.length() >= max_len) && hard_limit) {
        // still too long, truncate normally
        url_compact = QString(str_url);
        auto to_remove = url_compact.length() - max_len + 3;
        url_compact.remove(url_compact.length() - to_remove - 1, to_remove);
        url_compact.append("...");
    }

    return url_compact;
}

static const QStringList s_units_si{ "KB", "MB", "GB", "TB" };
static const QStringList s_units_kibi{ "KiB", "MiB", "GiB", "TiB" };

QString StringUtils::humanReadableFileSize(double bytes, bool use_si, int decimal_points)
{
    const QStringList units = use_si ? s_units_si : s_units_kibi;
    const int scale = use_si ? 1000 : 1024;

    int u = -1;
    double r = pow(10, decimal_points);

    do {
        bytes /= scale;
        u++;
    } while (round(abs(bytes) * r) / r >= scale && u < units.length() - 1);

    return QString::number(bytes, 'f', 2) + " " + units[u];
}

QString StringUtils::getRandomAlphaNumeric()
{
    return QUuid::createUuid().toString(QUuid::Id128);
}

QPair<QString, QString> StringUtils::splitFirst(const QString& s, const QString& sep, Qt::CaseSensitivity cs)
{
    QString left, right;
    auto index = s.indexOf(sep, 0, cs);
    left = s.mid(0, index);
    right = s.mid(index + sep.length());
    return qMakePair(left, right);
}

QPair<QString, QString> StringUtils::splitFirst(const QString& s, QChar sep, Qt::CaseSensitivity cs)
{
    QString left, right;
    auto index = s.indexOf(sep, 0, cs);
    left = s.mid(0, index);
    right = s.mid(left.length() + 1);
    return qMakePair(left, right);
}

QPair<QString, QString> StringUtils::splitFirst(const QString& s, const QRegularExpression& re)
{
    QString left, right;
    QRegularExpressionMatch match;
    auto index = s.indexOf(re, 0, &match);
    left = s.mid(0, index);
    auto end = match.hasMatch() ? left.length() + match.capturedLength() : left.length() + 1;
    right = s.mid(end);
    return qMakePair(left, right);
}

static const QRegularExpression ulMatcher("<\\s*/\\s*ul\\s*>");

QString StringUtils::htmlListPatch(QString htmlStr)
{
    int pos = htmlStr.indexOf(ulMatcher);
    int imgPos;
    while (pos != -1) {
        pos = htmlStr.indexOf(">", pos) + 1;  // Get the size of the </ul> tag. Add one for zeroeth index
        imgPos = htmlStr.indexOf("<img ", pos);
        if (imgPos == -1)
            break;  // no image after the tag

        auto textBetween = htmlStr.mid(pos, imgPos - pos).trimmed();  // trim all white spaces

        if (textBetween.isEmpty())
            htmlStr.insert(pos, "<br>");

        pos = htmlStr.indexOf(ulMatcher, pos);
    }
    return htmlStr;
}