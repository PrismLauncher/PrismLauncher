#include <QDebug>
#include <QString>
#include "sys.h"
#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#endif
#include <QFile>
#include <QMap>
#include <QProcess>
#include <QStandardPaths>

#ifdef Q_OS_MACOS
bool rosettaDetect()
{
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
        return false;
    }
    return ret == 1;
}
#endif

namespace SysInfo {
QString currentSystem()
{
#if defined(Q_OS_LINUX)
    return "linux";
#elif defined(Q_OS_MACOS)
    return "osx";
#elif defined(Q_OS_WINDOWS)
    return "windows";
#elif defined(Q_OS_FREEBSD)
    return "freebsd";
#elif defined(Q_OS_OPENBSD)
    return "openbsd";
#else
    return "unknown";
#endif
}

QString useQTForArch()
{
#if defined(Q_OS_MACOS) && !defined(Q_PROCESSOR_ARM)
    if (rosettaDetect()) {
        return "arm64";
    } else {
        return "x86_64";
    }
#endif
    return QSysInfo::currentCpuArchitecture();
}

int suitableMaxMem()
{
    float totalRAM = (float)Sys::getSystemRam() / (float)Sys::mebibyte;
    int maxMemoryAlloc;

    // If totalRAM < 6GB, use (totalRAM / 1.5), else 4GB
    if (totalRAM < (4096 * 1.5))
        maxMemoryAlloc = (int)(totalRAM / 1.5);
    else
        maxMemoryAlloc = 4096;

    return maxMemoryAlloc;
}

QString getSupportedJavaArchitecture()
{
    auto sys = currentSystem();
    auto arch = useQTForArch();
    if (sys == "windows") {
        if (arch == "x86_64")
            return "windows-x64";
        if (arch == "i386")
            return "windows-x86";
        // Unknown, maybe arm, appending arch
        return "windows-" + arch;
    }
    if (sys == "osx") {
        if (arch == "arm64")
            return "mac-os-arm64";
        if (arch.contains("64"))
            return "mac-os-x64";
        if (arch.contains("86"))
            return "mac-os-x86";
        // Unknown, maybe something new, appending arch
        return "mac-os-" + arch;
    } else if (sys == "linux") {
        if (arch == "x86_64")
            return "linux-x64";
        if (arch == "i386")
            return "linux-x86";
        // will work for arm32 arm(64)
        return "linux-" + arch;
    }
    return {};
}
}  // namespace SysInfo
