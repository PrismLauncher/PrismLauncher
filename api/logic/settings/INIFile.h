/* Copyright 2013-2019 MultiMC Contributors
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

#include <QString>
#include <QVariant>
#include <QIODevice>

#include "multimc_logic_export.h"

// Sectionless INI parser (for instance config files)
class MULTIMC_LOGIC_EXPORT INIFile : public QMap<QString, QVariant>
{
public:
    explicit INIFile();

    bool loadFile(QByteArray file);
    bool loadFile(QString fileName);
    bool saveFile(QString fileName);

    QVariant get(QString key, QVariant def) const;
    void set(QString key, QVariant val);
    static QString unescape(QString orig);
    static QString escape(QString orig);
};
