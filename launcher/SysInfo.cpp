#include <QDebug>
#include <QString>
#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#endif
#include <QFile>
#include <QMap>
#include <QProcess>
#include <QStandardPaths>
#include "Application.h"
#include "Commandline.h"
#include "java/JavaUtils.h"

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
}  // namespace SysInfo
