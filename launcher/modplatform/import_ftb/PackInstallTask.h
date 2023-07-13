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
 */

#pragma once

#include <QFuture>
#include <QFutureWatcher>

#include "InstanceTask.h"
#include "PackHelpers.h"

namespace FTBImportAPP {

class PackInstallTask : public InstanceTask {
    Q_OBJECT

   public:
    explicit PackInstallTask(const Modpack& pack) : m_pack(pack) {}
    virtual ~PackInstallTask() = default;

   protected:
    virtual void executeTask() override;

   private slots:
    void copySettings();

   private:
    QFuture<bool> m_copyFuture;
    QFutureWatcher<bool> m_copyFutureWatcher;

    const Modpack m_pack;
};

}  // namespace FTBImportAPP
