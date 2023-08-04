#include "sys.h"

#include "distroutils.h"

#include <sys/utsname.h>
#include <fstream>
#include <limits>

#include <QDebug>
#include <QString>
#include <QStringList>

Sys::KernelInfo Sys::getKernelInfo()
{
    Sys::KernelInfo out;
    struct utsname buf;
    uname(&buf);
    // NOTE: we assume linux here. this needs further elaboration
    out.kernelType = KernelType::Linux;
    out.kernelName = buf.sysname;
    QString release = out.kernelVersion = buf.release;

    // linux binary running on WSL is cursed.
    out.isCursed = release.contains("WSL", Qt::CaseInsensitive) || release.contains("Microsoft", Qt::CaseInsensitive);

    out.kernelMajor = 0;
    out.kernelMinor = 0;
    out.kernelPatch = 0;
    auto sections = release.split('-');
    if (sections.size() >= 1) {
        auto versionParts = sections[0].split('.');
        if (versionParts.size() >= 3) {
            out.kernelMajor = versionParts[0].toInt();
            out.kernelMinor = versionParts[1].toInt();
            out.kernelPatch = versionParts[2].toInt();
        } else {
            qWarning() << "Not enough version numbers in " << sections[0] << " found " << versionParts.size();
        }
    } else {
        qWarning() << "Not enough '-' sections in " << release << " found " << sections.size();
    }
    return out;
}

uint64_t Sys::getSystemRam()
{
    std::string token;
#ifdef Q_OS_LINUX
    std::ifstream file("/proc/meminfo");
    while (file >> token) {
        if (token == "MemTotal:") {
            uint64_t mem;
            if (file >> mem) {
                return mem * 1024ull;
            } else {
                return 0;
            }
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
#elif defined(Q_OS_FREEBSD)
    char buff[512];
    FILE* fp = popen("sysctl hw.physmem", "r");
    if (fp != NULL) {
        while (fgets(buff, 512, fp) != NULL) {
            std::string str(buff);
            uint64_t mem = std::stoull(str.substr(12, std::string::npos));
            return mem * 1024ull;
        }
    }
#endif
    return 0;  // nothing found
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
    DistributionInfo systemd_info = read_os_release();
    DistributionInfo lsb_info = read_lsb_release();
    DistributionInfo legacy_info = read_legacy_release();
    DistributionInfo result = systemd_info + lsb_info + legacy_info;
    if (result.distributionName.isNull()) {
        result.distributionName = "unknown";
    }
    if (result.distributionVersion.isNull()) {
        if (result.distributionName == "arch") {
            result.distributionVersion = "rolling";
        } else {
            result.distributionVersion = "unknown";
        }
    }
    return result;
}
