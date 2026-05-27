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

#include <QDir>
#include <QProcessEnvironment>

#include "Application.h"
#include "BuildConfig.h"

QStringList HardwareInfo::gpuInfo()
{
    QProcess process;

    auto exeName = QStringLiteral("%1_gpudetect").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#ifdef Q_OS_WIN32
    exeName.append(".exe");

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("__COMPAT_LAYER", "RUNASINVOKER");
    process.setProcessEnvironment(env);
#endif

    const QDir appExePath(APPLICATION->applicationDirPath());
    const QString pathToExe = appExePath.absoluteFilePath(exeName);

    process.setProgram(pathToExe);

    QStringList methods;
    if (QProcessEnvironment::systemEnvironment().value(QStringLiteral("%1_DISABLE_GLVULKAN").arg(BuildConfig.LAUNCHER_ENVNAME)).isEmpty()) {
        methods << "opengl";
#ifndef Q_OS_MACOS
        methods << "vulkan";
#endif
    }

#ifdef Q_OS_LINUX
    methods << "lspci";
#endif

    QStringList out;

    for (const auto& method : methods) {
        qInfo() << "Attempting GPU detection using" << method;

        process.setArguments({ method });
        process.start();
        if (!process.waitForFinished(1000)) {
            process.kill();
            qWarning() << "GPU detection process exited with code:" << process.exitCode() << process.errorString();
            continue;
        }

        for (const auto& str : process.readAllStandardOutput().split('\n')) {
            if (const auto line = str.trimmed(); !line.isEmpty()) {
                out << line;
            }
        }
        for (const auto& str : process.readAllStandardError().split('\n')) {
            if (const auto line = str.trimmed(); !line.isEmpty()) {
                qWarning() << line;
            }
        }
    }

    return out;
}

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
    return 0;
}

MacOSHardwareInfo::MemoryPressureLevel MacOSHardwareInfo::memoryPressureLevel()
{
    uint32_t level;
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
    }
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
