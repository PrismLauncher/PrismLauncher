#include "sys.h"

#include <sys/utsname.h>

#include <QString>
#include <QStringList>
#include <QDebug>

Sys::KernelInfo Sys::getKernelInfo()
{
    Sys::KernelInfo out;
    struct utsname buf;
    uname(&buf);
    out.kernelType = KernelType::Darwin;
    out.kernelName = buf.sysname;
    QString release = out.kernelVersion = buf.release;

    // TODO: figure out how to detect cursed-ness (macOS emulated on linux via mad hacks and so on)
    out.isCursed = false;

    out.kernelMajor = 0;
    out.kernelMinor = 0;
    out.kernelPatch = 0;
    auto sections = release.split('-');
    if(sections.size() >= 1) {
        auto versionParts = sections[0].split('.');
        if(versionParts.size() >= 3) {
            out.kernelMajor = versionParts[0].toInt();
            out.kernelMinor = versionParts[1].toInt();
            out.kernelPatch = versionParts[2].toInt();
        }
        else {
            qWarning() << "Not enough version numbers in " << sections[0] << " found " << versionParts.size();
        }
    }
    else {
        qWarning() << "Not enough '-' sections in " << release << " found " << sections.size();
    }
    return out;
}

#include <sys/sysctl.h>

uint64_t Sys::getSystemRam()
{
    uint64_t memsize;
    size_t memsizesize = sizeof(memsize);
    if(!sysctlbyname("hw.memsize", &memsize, &memsizesize, NULL, 0))
    {
        return memsize;
    }
    else
    {
        return 0;
    }
}

bool Sys::isCPU64bit()
{
    // not even going to pretend I'm going to support anything else
    return true;
}

bool Sys::isSystem64bit()
{
    // yep. maybe when we have 128bit CPUs on consumer devices.
    return true;
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
    DistributionInfo result;
    return result;
}
