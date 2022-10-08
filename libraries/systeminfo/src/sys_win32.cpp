#include "sys.h"

#include <windows.h>

Sys::KernelInfo Sys::getKernelInfo()
{
    Sys::KernelInfo out;
    out.kernelType = KernelType::Windows;
    out.kernelName = "Windows";
    OSVERSIONINFOW osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&osvi);
    out.kernelVersion = QString("%1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    out.kernelMajor = osvi.dwMajorVersion;
    out.kernelMinor = osvi.dwMinorVersion;
    out.kernelPatch = osvi.dwBuildNumber;
    return out;
}

uint64_t Sys::getSystemRam()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx( &status );
    // bytes
    return (uint64_t)status.ullTotalPhys;
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
    DistributionInfo result;
    return result;
}
