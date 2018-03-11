#include "sys.h"

#include <sys/utsname.h>

Sys::KernelInfo Sys::getKernelInfo()
{
	Sys::KernelInfo out;
	struct utsname buf;
	uname(&buf);
	out.kernelName = buf.sysname;
	out.kernelVersion = buf.release;
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
