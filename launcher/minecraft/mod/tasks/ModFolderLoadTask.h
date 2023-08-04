// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDir>
#include <QMap>
#include <QObject>
#include <QRunnable>
#include <memory>
#include "minecraft/mod/Mod.h"
#include "tasks/Task.h"

class ModFolderLoadTask : public Task {
    Q_OBJECT
   public:
    struct Result {
        QMap<QString, Mod::Ptr> mods;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const { return m_result; }

   public:
    ModFolderLoadTask(QDir mods_dir, QDir index_dir, bool is_indexed, bool clean_orphan = false);

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override
    {
        m_aborted.store(true);
        return true;
    }

    void executeTask() override;

   private:
    void getFromMetadata();

   private:
    QDir m_mods_dir, m_index_dir;
    bool m_is_indexed;
    bool m_clean_orphan;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;

    /** This is the thread in which we should put new mod objects */
    QThread* m_thread_to_spawn_into;
};
