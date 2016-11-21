#include "sys.h"

#include <sys/utsname.h>
#include <fstream>

QString Sys::getSystemInfo()
{
	struct utsname buf;
	uname(&buf);
	QString system(buf.sysname);
	QString release(buf.release);

	return system + "; " + release;
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
