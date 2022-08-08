#pragma once
#include <QString>

namespace Sys
{
const uint64_t mebibyte = 1024ull * 1024ull;

enum class KernelType {
    Undetermined,
    Windows,
    Darwin,
    Linux
};

struct KernelInfo
{
    QString kernelName;
    QString kernelVersion;

    KernelType kernelType = KernelType::Undetermined;
    int kernelMajor = 0;
    int kernelMinor = 0;
    int kernelPatch = 0;
    bool isCursed = false;
};

KernelInfo getKernelInfo();

struct DistributionInfo
{
    DistributionInfo operator+(const DistributionInfo& rhs) const
    {
        DistributionInfo out;
        if(!distributionName.isEmpty())
        {
            out.distributionName = distributionName;
        }
        else
        {
            out.distributionName = rhs.distributionName;
        }
        if(!distributionVersion.isEmpty())
        {
            out.distributionVersion = distributionVersion;
        }
        else
        {
            out.distributionVersion = rhs.distributionVersion;
        }
        return out;
    }
    QString distributionName;
    QString distributionVersion;
};

DistributionInfo getDistributionInfo();

uint64_t getSystemRam();
}
