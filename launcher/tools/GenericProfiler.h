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

#include "BaseProfiler.h"

class GenericProfilerFactory : public BaseProfilerFactory {
   public:
    QString name() const override { return "Generic"; }
    void registerSettings([[maybe_unused]] SettingsObjectPtr settings) override {};
    BaseExternalTool* createTool(InstancePtr instance, QObject* parent = 0) override;
    bool check([[maybe_unused]] QString* error) override { return true; };
    bool check([[maybe_unused]] const QString& path, [[maybe_unused]] QString* error) override { return true; };
};
