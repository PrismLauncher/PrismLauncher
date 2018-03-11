#pragma once
#include <QString>

namespace Sys
{
const uint64_t megabyte = 1024ull * 1024ull;
struct KernelInfo
{
	QString kernelName;
	QString kernelVersion;
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

bool isSystem64bit();

bool isCPU64bit();
}
