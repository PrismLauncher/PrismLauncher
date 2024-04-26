// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Joshua Goins <josh@redstrate.com>
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
 */

#include "Markdown.h"

QString markdownToHTML(const QString& markdown)
{
    const QByteArray markdownData = markdown.toUtf8();
    char* buffer = cmark_markdown_to_html(markdownData.constData(), markdownData.length(), CMARK_OPT_NOBREAKS | CMARK_OPT_UNSAFE);

    QString htmlStr(buffer);

    free(buffer);

    int pos = htmlStr.indexOf("</ul>");
    int imgPos;
    while (pos != -1) {
        pos = pos + 5;  // 5 is the size of the </ul> tag
        imgPos = htmlStr.indexOf("<img", pos);
        if (imgPos == -1)
            break;  // no image after the tag

        auto textBetween = htmlStr.mid(pos, imgPos - pos).trimmed();  // trim all white spaces

        if (textBetween.isEmpty())
            htmlStr.insert(pos, "<br>");

        pos = htmlStr.indexOf("</ul>", pos);
    }


    return htmlStr;
}