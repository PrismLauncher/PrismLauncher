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

#include "tasks/Task.h"

class BasicJavaDownloader : public Task {
    Q_OBJECT
   public:
    BasicJavaDownloader(QString final_path, bool m_is_legacy = false, QObject* parent = nullptr);
    virtual ~BasicJavaDownloader() = default;

    [[nodiscard]] bool canAbort() const override { return true; }

    virtual QString name() const = 0;
    virtual bool isSupported() const = 0;

   protected:
    QString m_os_name;
    QString m_os_arch;
    QString m_final_path;
    bool m_is_legacy;

    Task::Ptr m_current_task;
};