// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "lspci.h"

#include "Print.h"

#ifndef Q_OS_LINUX
void lspci::print()
{
    Print::err("lspci is supported only for Linux");
}
#else
#include <array>
#include <cstdio>

namespace {
QString afterColon(QString str)
{
    return str.remove(0, str.indexOf(':') + 2).trimmed();
}
}  // namespace

void lspci::print()
{
    std::array<char, 512> buffer{};
    FILE* lspci = popen("lspci -k", "r");  // NOLINT(*-command-processor)

    if (!lspci) {
        Print::err("lspci is not present");
        return;
    }

    bool readingGpuInfo = false;
    QString gpu;
    QString driverInUse = "NONE";
    QString driversAvailable;
    while (fgets(buffer.data(), 512, lspci) != nullptr) {
        const QString str(buffer.data());
        // clang-format off
        // 04:00.0 VGA compatible controller: Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 470/480/570/570X/580/580X/590] (rev e7)
        // Subsystem: Sapphire Technology Limited Radeon RX 580 Pulse 4GB
        // Kernel driver in use: amdgpu
        // Kernel modules: amdgpu
        // clang-format on
        if (str.contains("VGA compatible controller")) {
            readingGpuInfo = true;
        } else if (!str.startsWith('\t')) {
            if (readingGpuInfo) {
                Print::info(QString("Found VGA compatible controller: %1 (using driver %2, available drivers: %3)")
                                .arg(gpu, driverInUse, driversAvailable));
            }
            readingGpuInfo = false;
        }

        if (!readingGpuInfo) {
            continue;
        }

        const QString value = afterColon(str);
        if (str.contains("Subsystem")) {
            gpu = value;
        }
        if (str.contains("Kernel driver in use")) {
            driverInUse = value;
        }
        if (str.contains("Kernel modules")) {
            driversAvailable = value;
        }
    }

    pclose(lspci);
}
#endif
