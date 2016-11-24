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
		// We support only Windows NT (XP and up) - everything else is not interesting.
		OSVERSIONINFOW osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
		GetVersionExW(&osvi);
		QString os = QString("Windows NT %1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);

#if defined(_WIN64)
		// 64-bit programs run only on Win64
		os.append("; Win64");
		// determine CPU type
		SYSTEM_INFO info;
		ZeroMemory(&info, sizeof(SYSTEM_INFO));
		GetNativeSystemInfo(&info);
		auto arch = info.wProcessorArchitecture;
		if(arch == PROCESSOR_ARCHITECTURE_AMD64)
		{
			os.append("; x64");
		}
		else if (arch == PROCESSOR_ARCHITECTURE_IA64)
		{
			os.append("; IA64");
		}
#elif defined(_WIN32)
		// 32-bit programs run on both 32-bit and 64-bit Windows
		// so must sniff
		BOOL f64 = false;
		if(IsWow64Process(GetCurrentProcess(), &f64) && f64)
		{
			os.append("; WOW64");
		}
#endif
		return os;
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
