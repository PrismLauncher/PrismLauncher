/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "ATLPackManifest.h"

#include <QList>
#include <QMetaType>
#include <QString>

namespace ATLauncher {

struct IndexedVersion {
    QString version;
    QString minecraft;
};

struct IndexedPack {
    int id;
    int position;
    QString name;
    PackType type;
    QList<IndexedVersion> versions;
    bool system;
    QString description;

    QString safeName;
};

void loadIndexedPack(IndexedPack& m, QJsonObject& obj);
}  // namespace ATLauncher

Q_DECLARE_METATYPE(ATLauncher::IndexedPack)
Q_DECLARE_METATYPE(QList<ATLauncher::IndexedVersion>)
