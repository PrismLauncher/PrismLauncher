// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Toshit Chawda <r58playz@gmail.com>
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
 */

#include "LaunchContext.h"

LaunchContext::LaunchContext(SettingsObjectPtr instanceSettings){
    m_instanceSettings = instanceSettings;
}

void LaunchContext::setRealJavaArch(QVariant realJavaArch)
{
    m_instanceSettings->set("JavaRealArchitecture", realJavaArch);
}

QVariant LaunchContext::getRealJavaArch()
{
    return m_instanceSettings->get("JavaRealArchitecture");
}

QVariant LaunchContext::getJavaPath()
{
    return m_instanceSettings->get("JavaPath");
}
