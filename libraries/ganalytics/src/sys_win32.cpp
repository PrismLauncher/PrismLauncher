#include "sys.h"

// FIXME: replace with our version...
QString Sys::getSystemInfo()
{
	QSysInfo::WinVersion version = QSysInfo::windowsVersion();
	QString os("Windows; ");
	switch (version)
	{
	case QSysInfo::WV_95:
		os += "Win 95";
		break;
	case QSysInfo::WV_98:
		os += "Win 98";
		break;
	case QSysInfo::WV_Me:
		os += "Win ME";
		break;
	case QSysInfo::WV_NT:
		os += "Win NT";
		break;
	case QSysInfo::WV_2000:
		os += "Win 2000";
		break;
	case QSysInfo::WV_2003:
		os += "Win Server 2003";
		break;
	case QSysInfo::WV_VISTA:
		os += "Win Vista";
		break;
	case QSysInfo::WV_WINDOWS7:
		os += "Win 7";
		break;
	case QSysInfo::WV_WINDOWS8:
		os += "Win 8";
		break;
	case QSysInfo::WV_WINDOWS8_1:
		os += "Win 8.1";
		break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QSysInfo::WV_WINDOWS10:
		os += "Win 10";
		break;
#endif
	default:
		os = "Windows; unknown";
		break;
	}
	return os;
}

#include <windows.h>

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
	return true;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
	// 32-bit programs run on both 32-bit and 64-bit Windows
	// so must sniff
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
	GetNativeSystemInfo(&info);
	auto arch = info.wProcessorArchitecture;
	return arch == PROCESSOR_ARCHITECTURE_AMD64 || arch == PROCESSOR_ARCHITECTURE_IA64;
}
