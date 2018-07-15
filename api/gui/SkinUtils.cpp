/* Copyright 2013-2018 MultiMC Contributors
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

#include "SkinUtils.h"
#include "net/HttpMetaCache.h"
#include "Env.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace SkinUtils
{
/*
 * Given a username, return a pixmap of the cached skin (if it exists), QPixmap() otherwise
 */
QPixmap getFaceFromCache(QString username, int height, int width)
{
    QFile fskin(ENV.metacache()
                    ->resolveEntry("skins", username + ".png")
                    ->getFullPath());

    if (fskin.exists())
    {
        QPixmap skin(fskin.fileName());
        if(!skin.isNull())
        {
            return skin.copy(8, 8, 8, 8).scaled(height, width, Qt::KeepAspectRatio);
        }
    }

    return QPixmap();
}
}
