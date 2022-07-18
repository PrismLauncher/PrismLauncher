// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QList>

#include "QObjectPtr.h"
#include "ModDetails.h"

class Mod : public QObject
{
    Q_OBJECT
public:
    enum ModType
    {
        MOD_UNKNOWN,    //!< Indicates an unspecified mod type.
        MOD_ZIPFILE,    //!< The mod is a zip file containing the mod's class files.
        MOD_SINGLEFILE, //!< The mod is a single file (not a zip file).
        MOD_FOLDER,     //!< The mod is in a folder on the filesystem.
        MOD_LITEMOD,    //!< The mod is a litemod
    };

    using Ptr = shared_qobject_ptr<Mod>;

    Mod() = default;
    Mod(const QFileInfo &file);
    explicit Mod(const QDir& mods_dir, const Metadata::ModStruct& metadata);

    auto fileinfo()        const -> QFileInfo { return m_file; }
    auto dateTimeChanged() const -> QDateTime { return m_changedDateTime; }
    auto internal_id()     const -> QString { return m_internal_id; }
    auto type()            const -> ModType { return m_type; }
    auto enabled()         const -> bool { return m_enabled; }

    auto valid() const -> bool { return m_type != MOD_UNKNOWN; }

    auto details()     const -> const ModDetails&;
    auto name()        const -> QString;
    auto version()     const -> QString;
    auto homeurl()     const -> QString;
    auto description() const -> QString;
    auto authors()     const -> QStringList;
    auto status()      const -> ModStatus;

    auto metadata() -> std::shared_ptr<Metadata::ModStruct>;
    auto metadata() const -> const std::shared_ptr<Metadata::ModStruct>;

    void setStatus(ModStatus status);
    void setMetadata(const Metadata::ModStruct& metadata);

    auto enable(bool value) -> bool;

    // delete all the files of this mod
    auto destroy(QDir& index_dir, bool preserve_metadata = false) -> bool;

    // change the mod's filesystem path (used by mod lists for *MAGIC* purposes)
    void repath(const QFileInfo &file);

    auto shouldResolve()    const -> bool { return !m_resolving && !m_resolved; }
    auto isResolving()      const -> bool { return m_resolving; }
    auto resolutionTicket() const -> int  { return m_resolutionTicket; }

    void setResolving(bool resolving, int resolutionTicket) {
        m_resolving = resolving;
        m_resolutionTicket = resolutionTicket;
    }
    void finishResolvingWithDetails(std::shared_ptr<ModDetails> details);

protected:
    QFileInfo m_file;
    QDateTime m_changedDateTime;

    QString m_internal_id;
    /* Name as reported via the file name */
    QString m_name;
    ModType m_type = MOD_UNKNOWN;

    /* If the mod has metadata, this will be filled in the constructor, and passed to 
     * the ModDetails when calling finishResolvingWithDetails */
    std::shared_ptr<Metadata::ModStruct> m_temp_metadata;

    /* Set the mod status while it doesn't have local details just yet */
    ModStatus m_temp_status = ModStatus::NoMetadata;

    std::shared_ptr<ModDetails> m_localDetails;

    bool m_enabled = true;
    bool m_resolving = false;
    bool m_resolved = false;
    int m_resolutionTicket = 0;
};
