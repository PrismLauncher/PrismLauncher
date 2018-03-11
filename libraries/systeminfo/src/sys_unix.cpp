#include "sys.h"

#include "distroutils.h"

#include <sys/utsname.h>
#include <fstream>

Sys::KernelInfo Sys::getKernelInfo()
{
	Sys::KernelInfo out;
	struct utsname buf;
	uname(&buf);
	out.kernelName = buf.sysname;
	out.kernelVersion = buf.release;
	return out;
}

uint64_t Sys::getSystemRam()
{
	std::string token;
	std::ifstream file("/proc/meminfo");
	while(file >> token)
	{
		if(token == "MemTotal:")
		{
			uint64_t mem;
			if(file >> mem)
			{
				return mem * 1024ull;
			}
			else
			{
				return 0;
			}
		}
		// ignore rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // nothing found
}

bool Sys::isCPU64bit()
{
	return isSystem64bit();
}

bool Sys::isSystem64bit()
{
	// kernel build arch on linux
	return QSysInfo::currentCpuArchitecture() == "x86_64";
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
	DistributionInfo systemd_info = read_os_release();
	DistributionInfo lsb_info = read_lsb_release();
	DistributionInfo legacy_info = read_legacy_release();
	DistributionInfo result = systemd_info + lsb_info + legacy_info;
	if(result.distributionName.isNull())
	{
		result.distributionName = "unknown";
	}
	if(result.distributionVersion.isNull())
	{
		if(result.distributionName == "arch")
		{
			result.distributionVersion = "rolling";
		}
		else
		{
			result.distributionVersion = "unknown";
		}
	}
	return result;
}
