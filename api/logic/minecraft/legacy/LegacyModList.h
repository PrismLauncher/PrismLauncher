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

#include <QList>
#include <QString>
#include <QDir>

#include "minecraft/Mod.h"

#include "multimc_logic_export.h"

class LegacyInstance;
class BaseInstance;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class MULTIMC_LOGIC_EXPORT LegacyModList
{
public:

    LegacyModList(const QString &dir, const QString &list_file = QString());

    /// Reloads the mod list and returns true if the list changed.
    bool update();

    QDir dir()
    {
        return m_dir;
    }

    const QList<Mod> & allMods()
    {
        return mods;
    }

protected:
    QDir m_dir;
    QString m_list_file;
    QList<Mod> mods;
};
