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

#include <QDebug>
#include <QStringList>

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
namespace {
QString afterColon(QString str)
{
    return str.remove(0, str.indexOf(':') + 2).trimmed();
}

template <typename F>
bool readFromOutput(const char* command, F function)
{
    FILE* file = popen(command, "r");  // NOLINT(*-command-processor)
    if (!file) {
        qWarning().nospace() << "Could not execute command '" << command << "': " << strerror(errno);
        return false;
    }

    constexpr size_t bufferSize = 512;
    std::array<char, bufferSize> buffer{};
    while (fgets(buffer.data(), bufferSize, file) != nullptr) {
        function(buffer.data());
    }

    const int exitCode = pclose(file);
    if (exitCode != 0) {
        if (exitCode == -1) {
            qWarning().nospace() << "Could not close stream for command '" << command << "': " << strerror(errno);
        } else {
            qWarning().nospace() << "Command '" << command << "' exited with code " << exitCode;
        }

        return false;
    }

    return true;
}
}  // namespace
#endif

#ifdef Q_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <QSettings>

#include <dxgi1_6.h>
#include <windows.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

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

QStringList HardwareInfo::gpuInfo()
{
    ComPtr<IDXGIFactory6> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        qWarning() << "Could not create DXGI factory:" << Qt::hex << hr;
        return { "GPU discovery failed: could not create DXGI factory" };
    }

    UINT i = 0;
    ComPtr<IDXGIAdapter> adapter;
    QStringList out;
    while (factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        hr = adapter->GetDesc(&desc);
        if (SUCCEEDED(hr)) {
            out << "GPU: " + QString::fromWCharArray(desc.Description);  // NOLINT(*-pro-bounds-array-to-pointer-decay, *-no-array-decay)
        } else {
            qWarning() << "Could not get DXGI adapter description:" << Qt::hex << hr;
        }

        ++i;
    }

    return out;
}

#elif defined(Q_OS_MACOS)
#include "sys/sysctl.h"

QString HardwareInfo::cpuInfo()
{
    std::array<char, 512> buffer{};
    size_t bufferSize = buffer.size();
    if (sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferSize, nullptr, 0) == 0) {
        return { buffer.data() };
    }

    qWarning() << "Could not get CPU model: sysctlbyname";
    return "";
}

uint64_t HardwareInfo::totalRamMiB()
{
    uint64_t memsize = 0;
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
    return 0;
}

MacOSHardwareInfo::MemoryPressureLevel MacOSHardwareInfo::memoryPressureLevel()
{
    uint32_t level = 0;
    size_t levelSize = sizeof level;
    if (sysctlbyname("kern.memorystatus_vm_pressure_level", &level, &levelSize, nullptr, 0) == 0) {
        return static_cast<MemoryPressureLevel>(level);
    }

    qWarning() << "Could not get memory pressure level: sysctlbyname";
    return MemoryPressureLevel::Normal;
}

QString MacOSHardwareInfo::memoryPressureLevelName()
{
    // The names are internal, users refer to levels by their graph colors in Activity Monitor
    switch (memoryPressureLevel()) {
        case MemoryPressureLevel::Normal:
            return "Green";
        case MemoryPressureLevel::Warning:
            return "Yellow";
        case MemoryPressureLevel::Critical:
            return "Red";
        default:
            Q_ASSERT(false);
            return "";
    }
}

QStringList HardwareInfo::gpuInfo()
{
    QStringList out;
    const bool success = readFromOutput("system_profiler SPDisplaysDataType", [&](const QString& str) {
        // Chipset Model: Intel HD Graphics 620
        if (str.contains("Chipset Model")) {
            out << "GPU: " + afterColon(str);
        }
    });
    if (!success) {
        return { "GPU discovery failed: could not read from system_profiler" };
    }

    return out;
}

#elif defined(Q_OS_LINUX)
#include <fstream>

QString HardwareInfo::cpuInfo()
{
    std::ifstream cpuin("/proc/cpuinfo");
    for (std::string line; std::getline(cpuin, line);) {
        // model name      : AMD Ryzen 7 5800X 8-Core Processor
        if (const QString str = QString::fromStdString(line); str.startsWith("model name")) {
            return afterColon(str);
        }
    }

    qWarning() << "Could not get CPU model: /proc/cpuinfo";
    return "unknown";
}

namespace {
uint64_t readMemInfo(const QString& searchTarget)
{
    std::ifstream memin("/proc/meminfo");
    for (std::string line; std::getline(memin, line);) {
        // MemTotal:       16287480 kB
        if (const QString str = QString::fromStdString(line); str.startsWith(searchTarget)) {
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
}  // namespace

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
    bool readingGpuInfo = false;
    QString gpu;
    QString driverInUse = "NONE";
    QString driversAvailable = "NONE";
    QStringList out;

    const bool success = readFromOutput("lspci -k", [&](const QString& str) {
        // clang-format off
        // 04:00.0 VGA compatible controller: Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 470/480/570/570X/580/580X/590] (rev e7)
        // Subsystem: Sapphire Technology Limited Radeon RX 580 Pulse 4GB
        // Kernel driver in use: amdgpu
        // Kernel modules: amdgpu
        // clang-format on
        if (str.contains("VGA compatible controller") || str.contains("3D controller")) {
            readingGpuInfo = true;
        } else if (!str.startsWith('\t')) {
            if (readingGpuInfo) {
                out << QString("GPU: %1 (driver in use: %2; drivers available: %3)").arg(gpu, driverInUse, driversAvailable);
                driverInUse = "NONE";
                driversAvailable = "NONE";
            }
            readingGpuInfo = false;
        }

        if (!readingGpuInfo) {
            return;
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
    });
    if (!success) {
        return { "GPU discovery failed: could not read from lspci" };
    }

    return out;
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
    uint64_t out = 0;

    const bool success = readFromOutput("sysctl hw.physmem", [&](const QString& str) {
        const uint64_t mem = str.mid(12).toULong();

        // transforming kib -> mib
        out = mem / 1024;
    });
    if (!success) {
        qWarning() << "Could not get total RAM: could not read from sysctl";
        return 0;
    }

    return out;
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

QStringList HardwareInfo::gpuInfo()
{
    return { "GPU discovery failed: not implemented for this OS" };
}
#endif
