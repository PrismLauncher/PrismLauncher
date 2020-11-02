/* Copyright 2020 MultiMC Contributors
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

#pragma once

#include <QList>
#include <QString>

namespace Technic {
struct Modpack {
    QString slug;

    QString name;
    QString logoUrl;
    QString logoName;

    bool broken = true;

    QString url;
    bool isSolder = false;
    QString minecraftVersion;

    bool metadataLoaded = false;
    QString websiteUrl;
    QString author;
    QString description;
};
}

Q_DECLARE_METATYPE(Technic::Modpack)
