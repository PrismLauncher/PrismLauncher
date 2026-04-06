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

#include "EnsureAvailableMemory.h"

#include "HardwareInfo.h"
#include "ui/dialogs/CustomMessageBox.h"

EnsureAvailableMemory::EnsureAvailableMemory(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance) {}

void EnsureAvailableMemory::executeTask()
{
    const uint64_t available = HardwareInfo::availableRamMiB();
    const uint64_t min = m_instance->settings()->get("MinMemAlloc").toUInt();
    const uint64_t max = m_instance->settings()->get("MaxMemAlloc").toUInt();
    const uint64_t required = std::max(min, max);

    if (required > available) {
        auto* dialog = CustomMessageBox::selectable(
            nullptr, tr("Not enough RAM"),
            tr("There is not enough RAM available to launch this instance with the current memory settings.\n\n"
               "Required: %1 MiB\nAvailable: %2 MiB\n\n"
               "Continue anyway? This may cause slowdowns in the game and your system.")
                .arg(required)
                .arg(available),
            QMessageBox::Icon::Warning, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
            QMessageBox::StandardButton::No);
        const auto response = dialog->exec();
        dialog->deleteLater();

        const auto message = tr("Not enough RAM available to launch this instance");
        if (response == QMessageBox::No) {
            emit logLine(message, MessageLevel::Fatal);
            emitFailed(message);
            return;
        }

        emit logLine(message, MessageLevel::Warning);
    }

    emitSucceeded();
}
