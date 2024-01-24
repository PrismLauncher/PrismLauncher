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
    if (ret == 0) {
        return false;
    }
    if (ret == 1) {
        return true;
    }
    return false;
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
    auto qtArch = QSysInfo::currentCpuArchitecture();
#if defined(Q_OS_MACOS) && !defined(Q_PROCESSOR_ARM)
    if (rosettaDetect()) {
        return "arm64";
    } else {
        return "x86_64";
    }
#endif
    return qtArch;
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
}  // namespace SysInfo
