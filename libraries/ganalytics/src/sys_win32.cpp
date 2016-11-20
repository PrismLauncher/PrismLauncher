#include "sys.h"

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

