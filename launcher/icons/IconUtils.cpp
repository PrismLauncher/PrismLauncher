// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "IconUtils.h"

#include <QDirIterator>
#include "FileSystem.h"

#include <array>

namespace {
std::array<const char*, 6> validIconExtensions = { { "svg", "png", "ico", "gif", "jpg", "jpeg" } };
}

namespace IconUtils {

QString findBestIconIn(const QString& folder, const QString& iconKey)
{
    int best_found = validIconExtensions.size();
    QString best_filename;

    QDirIterator it(folder, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        auto fileInfo = it.fileInfo();

        if (fileInfo.completeBaseName() != iconKey)
            continue;

        auto extension = fileInfo.suffix();

        for (int i = 0; i < best_found; i++) {
            if (extension == validIconExtensions[i]) {
                best_found = i;
                qDebug() << i << " : " << fileInfo.fileName();
                best_filename = fileInfo.fileName();
            }
        }
    }
    return FS::PathCombine(folder, best_filename);
}

QString getIconFilter()
{
    QString out;
    QTextStream stream(&out);
    stream << '(';
    for (size_t i = 0; i < validIconExtensions.size() - 1; i++) {
        if (i > 0) {
            stream << " ";
        }
        stream << "*." << validIconExtensions[i];
    }
    stream << " *." << validIconExtensions[validIconExtensions.size() - 1];
    stream << ')';
    return out;
}

}  // namespace IconUtils
