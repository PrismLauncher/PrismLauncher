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

#include "HardwareInfo.h"

#include <QCoreApplication>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QProcessEnvironment>
#include "BuildConfig.h"

#ifndef Q_OS_MACOS
#include <QVulkanInstance>
#include <QVulkanWindow>
#endif

namespace {
bool vulkanInfo(QStringList& out)
{
    if (!QProcessEnvironment::systemEnvironment()
             .value(QStringLiteral("%1_DISABLE_GLVULKAN").arg(BuildConfig.LAUNCHER_ENVNAME))
             .isEmpty()) {
        return false;
    }
#ifndef Q_OS_MACOS
    QVulkanInstance inst;
    if (!inst.create()) {
        qWarning() << "Vulkan instance creation failed, VkResult:" << inst.errorCode();
        out << "Couldn't get Vulkan device information";
        return false;
    }

    QVulkanWindow window;
    window.setVulkanInstance(&inst);

    for (auto device : window.availablePhysicalDevices()) {
        const auto supportedVulkanVersion = QVersionNumber(VK_API_VERSION_MAJOR(device.apiVersion), VK_API_VERSION_MINOR(device.apiVersion),
                                                           VK_API_VERSION_PATCH(device.apiVersion));
        out << QString("Found Vulkan device: %1 (API version %2)").arg(device.deviceName).arg(supportedVulkanVersion.toString());
    }
#endif

    return true;
}

bool openGlInfo(QStringList& out)
{
    if (!QProcessEnvironment::systemEnvironment()
             .value(QStringLiteral("%1_DISABLE_GLVULKAN").arg(BuildConfig.LAUNCHER_ENVNAME))
             .isEmpty()) {
        return false;
    }
    QOpenGLContext ctx;
    if (!ctx.create()) {
        qWarning() << "OpenGL context creation failed";
        out << "Couldn't get OpenGL device information";
        return false;
    }

    QOffscreenSurface surface;
    surface.create();
    ctx.makeCurrent(&surface);

    auto* f = ctx.functions();
    f->initializeOpenGLFunctions();

    auto toQString = [](const GLubyte* str) { return QString(reinterpret_cast<const char*>(str)); };
    out << "OpenGL driver vendor: " + toQString(f->glGetString(GL_VENDOR));
    out << "OpenGL renderer: " + toQString(f->glGetString(GL_RENDERER));
    out << "OpenGL driver version: " + toQString(f->glGetString(GL_VERSION));

    return true;
}
}  // namespace

#ifndef Q_OS_LINUX
QStringList HardwareInfo::gpuInfo()
{
    QStringList info;
    vulkanInfo(info);
    openGlInfo(info);
    return info;
}
#endif

#ifdef Q_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <QSettings>

#include "windows.h"

QString HardwareInfo::cpuInfo()
{
    const QSettings registry(R"(HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0)", QSettings::NativeFormat);
    return registry.value("ProcessorNameString").toString();
}

uint64_t HardwareInfo::totalRamMiB()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof status;

    if (GlobalMemoryStatusEx(&status) == TRUE) {
        // transforming bytes -> mib
        return status.ullTotalPhys / 1024 / 1024;
    }

    qWarning() << "Could not get total RAM: GlobalMemoryStatusEx";
    return 0;
}

uint64_t HardwareInfo::availableRamMiB()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof status;

    if (GlobalMemoryStatusEx(&status) == TRUE) {
        // transforming bytes -> mib
        return status.ullAvailPhys / 1024 / 1024;
    }

    qWarning() << "Could not get available RAM: GlobalMemoryStatusEx";
    return 0;
}

#elif defined(Q_OS_MACOS)
#include "mach/mach.h"
#include "sys/sysctl.h"

QString HardwareInfo::cpuInfo()
{
    std::array<char, 512> buffer;
    size_t bufferSize = buffer.size();
    if (sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferSize, nullptr, 0) == 0) {
        return QString(buffer.data());
    }

    qWarning() << "Could not get CPU model: sysctlbyname";
    return "";
}

uint64_t HardwareInfo::totalRamMiB()
{
    uint64_t memsize;
    size_t memsizeSize = sizeof memsize;
    if (sysctlbyname("hw.memsize", &memsize, &memsizeSize, nullptr, 0) == 0) {
        // transforming bytes -> mib
        return memsize / 1024 / 1024;
    }

    qWarning() << "Could not get total RAM: sysctlbyname";
    return 0;
}

uint64_t HardwareInfo::availableRamMiB()
{
    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

    vm_statistics64_data_t vm_stats;

    if (host_statistics64(host_port, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stats), &count) == KERN_SUCCESS) {
        // transforming bytes -> mib
        return (vm_stats.free_count + vm_stats.inactive_count) * vm_page_size / 1024 / 1024;
    }

    qWarning() << "Could not get available RAM: host_statistics64";
    return 0;
}

#elif defined(Q_OS_LINUX)
#include <fstream>

namespace {
QString afterColon(QString& str)
{
    return str.remove(0, str.indexOf(':') + 2).trimmed();
}
}  // namespace

QString HardwareInfo::cpuInfo()
{
    std::ifstream cpuin("/proc/cpuinfo");
    for (std::string line; std::getline(cpuin, line);) {
        // model name      : AMD Ryzen 7 5800X 8-Core Processor
        if (QString str = QString::fromStdString(line); str.startsWith("model name")) {
            return afterColon(str);
        }
    }

    qWarning() << "Could not get CPU model: /proc/cpuinfo";
    return "unknown";
}

uint64_t readMemInfo(QString searchTarget)
{
    std::ifstream memin("/proc/meminfo");
    for (std::string line; std::getline(memin, line);) {
        // MemTotal:       16287480 kB
        if (QString str = QString::fromStdString(line); str.startsWith(searchTarget)) {
            bool ok = false;
            const uint total = str.simplified().section(' ', 1, 1).toUInt(&ok);
            if (!ok) {
                qWarning() << "Could not read /proc/meminfo: failed to parse string:" << str;
                return 0;
            }

            // transforming kib -> mib
            return total / 1024;
        }
    }

    qWarning() << "Could not read /proc/meminfo: search target not found:" << searchTarget;
    return 0;
}

uint64_t HardwareInfo::totalRamMiB()
{
    return readMemInfo("MemTotal");
}

uint64_t HardwareInfo::availableRamMiB()
{
    return readMemInfo("MemAvailable");
}

QStringList HardwareInfo::gpuInfo()
{
    QStringList list;
    const bool vulkanSuccess = vulkanInfo(list);
    const bool openGlSuccess = openGlInfo(list);
    if (vulkanSuccess || openGlSuccess) {
        return list;
    }

    std::array<char, 512> buffer;
    FILE* lspci = popen("lspci -k", "r");

    if (!lspci) {
        return { "Could not detect GPUs: lspci is not present" };
    }

    bool readingGpuInfo = false;
    QString currentModel = "";
    while (fgets(buffer.data(), 512, lspci) != nullptr) {
        QString str(buffer.data());
        // clang-format off
        // 04:00.0 VGA compatible controller: Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 470/480/570/570X/580/580X/590] (rev e7)
        // Subsystem: Sapphire Technology Limited Radeon RX 580 Pulse 4GB
        // Kernel driver in use: amdgpu
        // Kernel modules: amdgpu
        // clang-format on
        if (str.contains("VGA compatible controller")) {
            readingGpuInfo = true;
        } else if (!str.startsWith('\t')) {
            readingGpuInfo = false;
        }
        if (!readingGpuInfo) {
            continue;
        }

        if (str.contains("Subsystem")) {
            currentModel = "Found GPU: " + afterColon(str);
        }
        if (str.contains("Kernel driver in use")) {
            currentModel += " (using driver " + afterColon(str);
        }
        if (str.contains("Kernel modules")) {
            currentModel += ", available drivers: " + afterColon(str) + ")";
            list.append(currentModel);
        }
    }
    pclose(lspci);
    return list;
}

#else

QString HardwareInfo::cpuInfo()
{
    return "unknown";
}

#if defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
#include <cstdio>

uint64_t HardwareInfo::totalRamMiB()
{
    char buff[512];
    FILE* fp = popen("sysctl hw.physmem", "r");
    if (fp != nullptr) {
        if (fgets(buff, 512, fp) != nullptr) {
            std::string str(buff);
            uint64_t mem = std::stoull(str.substr(12, std::string::npos));

            // transforming kib -> mib
            return mem / 1024;
        }
    }

    return 0;
}

#else
uint64_t HardwareInfo::totalRamMiB()
{
    return 0;
}
#endif

uint64_t HardwareInfo::availableRamMiB()
{
    return 0;
}

#endif
