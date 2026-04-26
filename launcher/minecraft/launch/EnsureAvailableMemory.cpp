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
#ifdef Q_OS_MACOS
    QString text;
    switch (MacOSHardwareInfo::memoryPressureLevel()) {
        case MacOSHardwareInfo::MemoryPressureLevel::Normal:
            emitSucceeded();
            return;
        case MacOSHardwareInfo::MemoryPressureLevel::Warning:
            text =
                tr("The system is under increased memory pressure.\n"
                   "This may lead to lag or slowdowns.\n"
                   "If possible, close other applications before continuing.\n\n"
                   "Launch anyway?");
            break;
        case MacOSHardwareInfo::MemoryPressureLevel::Critical:
            text =
                tr("Your system is under critical memory pressure.\n"
                   "This may lead to severe slowdowns, crashes or system instability.\n"
                   "It is recommended to close other applications or restart your system.\n\n"
                   "Launch anyway?");
            break;
    }

    bool shouldAbort = false;

    if (m_instance->settings()->get("LowMemWarning").toBool()) {
        auto* dialog = CustomMessageBox::selectable(nullptr, tr("High memory pressure"), text, QMessageBox::Icon::Warning,
                                                    QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                                                    QMessageBox::StandardButton::No);

        shouldAbort = dialog->exec() == QMessageBox::No;
        dialog->deleteLater();
    }

    const auto message = tr("The system is under high memory pressure");
    if (shouldAbort) {
        emit logLine(message, MessageLevel::Fatal);
        emitFailed(message);
        return;
    }

    emit logLine(message, MessageLevel::Warning);
    emitSucceeded();
#else
    const uint64_t available = HardwareInfo::availableRamMiB();
    if (available == 0) {
        // could not read
        emitSucceeded();
        return;
    }

    const uint64_t settingMin = m_instance->settings()->get("MinMemAlloc").toUInt();
    const uint64_t settingMax = m_instance->settings()->get("MaxMemAlloc").toUInt();
    const uint64_t max = std::max(settingMin, settingMax);

    if (static_cast<double>(max) * 0.9 > static_cast<double>(available)) {
        bool shouldAbort = false;

        if (m_instance->settings()->get("LowMemWarning").toBool()) {
            auto* dialog = CustomMessageBox::selectable(
                nullptr, tr("Low free memory"),
                tr("There might not be enough free RAM to launch this instance with the current memory settings.\n\n"
                   "Maximum allocated: %1 MiB\nFree: %2 MiB (out of %3 MiB total)\n\n"
                   "Launch anyway? This may cause slowdowns in the game and your system.")
                    .arg(max)
                    .arg(available)
                    .arg(HardwareInfo::totalRamMiB()),
                QMessageBox::Icon::Warning, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                QMessageBox::StandardButton::No);

            shouldAbort = dialog->exec() == QMessageBox::No;
            dialog->deleteLater();
        }

        const auto message = tr("Not enough RAM available to launch this instance");
        if (shouldAbort) {
            emit logLine(message, MessageLevel::Fatal);
            emitFailed(message);
            return;
        }

        emit logLine(message, MessageLevel::Warning);
    }

    emitSucceeded();
#endif
}
