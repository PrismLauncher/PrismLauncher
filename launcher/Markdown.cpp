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
    
    // Insert a breakpoint between a </ul> and <img> tag as this can cause visual bugs
    int first_pos = htmlStr.indexOf("</ul>");
    int img_pos;
    while (first_pos != -1) {
        img_pos = htmlStr.indexOf("<img", first_pos);

        if (img_pos - (first_pos + 5) < 3 && img_pos - (first_pos + 5) > -1)  // 5 is the size of the </ul> tag
            htmlStr.insert(img_pos, "<br>");

        first_pos = htmlStr.indexOf("</ul>", first_pos + 5);
    }

    return htmlStr;
}