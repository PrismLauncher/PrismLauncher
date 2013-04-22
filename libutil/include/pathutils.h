/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PATHUTILS_H
#define PATHUTILS_H

#include <QString>

#include "libutil_config.h"

LIBUTIL_EXPORT QString PathCombine(QString path1, QString path2);
LIBUTIL_EXPORT QString PathCombine(QString path1, QString path2, QString path3);

LIBUTIL_EXPORT QString AbsolutePath(QString path);

LIBUTIL_EXPORT QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

LIBUTIL_EXPORT QString DirNameFromString(QString string, QString inDir = ".");

#endif // PATHUTILS_H
