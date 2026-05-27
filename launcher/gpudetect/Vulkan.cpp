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

#include "Vulkan.h"

#include "Print.h"

#ifdef Q_OS_MACOS
void Vulkan::print()
{
    Print::err("Vulkan is not supported on macOS");
}
#else
#include <QVulkanWindow>

void Vulkan::print()
{
    QVulkanInstance inst;
    if (!inst.create()) {
        Print::err("Vulkan instance creation failed, VkResult: " + QString::number(inst.errorCode()));
        return;
    }

    QVulkanWindow window;
    window.setVulkanInstance(&inst);

    for (auto device : window.availablePhysicalDevices()) {
        const auto supportedVulkanVersion = QVersionNumber(VK_API_VERSION_MAJOR(device.apiVersion), VK_API_VERSION_MINOR(device.apiVersion),
                                                           VK_API_VERSION_PATCH(device.apiVersion));
        Print::info(QString("Found Vulkan device: %1 (API version %2)").arg(device.deviceName, supportedVulkanVersion.toString()));
    }
}
#endif
