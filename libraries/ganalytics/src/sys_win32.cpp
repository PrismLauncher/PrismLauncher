#include "sys.h"

#include <windows.h>

QString Sys::getSystemInfo()
{
	static QString cached;
	if(!cached.isNull())
	{
		return cached;
	}
	else
	{
		OSVERSIONINFOW osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
		GetVersionExW(&osvi);
		cached = QString("Windows %1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
		return cached;
	}
}

uint64_t Sys::getSystemRam()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx( &status );
	// bytes
	return (uint64_t)status.ullTotalPhys;
}

bool Sys::isSystem64bit()
{
#if defined(_WIN64)
	return true;
#elif defined(_WIN32)
	BOOL f64 = false;
	return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
	// it's some other kind of system...
	return false;
#endif
}

bool Sys::isCPU64bit()
{
	SYSTEM_INFO info;
	ZeroMemory(&info, sizeof(SYSTEM_INFO));
	GetNativeSystemInfo(&info);
	auto arch = info.wProcessorArchitecture;
	return arch == PROCESSOR_ARCHITECTURE_AMD64 || arch == PROCESSOR_ARCHITECTURE_IA64;
}
