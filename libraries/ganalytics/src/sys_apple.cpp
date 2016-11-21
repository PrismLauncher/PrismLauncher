#include "sys.h"

QString Sys::getSystemInfo()
{
	QSysInfo::MacVersion version = QSysInfo::macVersion();
	QString os;
	switch (version)
	{
	case QSysInfo::MV_9:
		os = "Macintosh; Mac OS 9";
		break;
	case QSysInfo::MV_10_0:
		os = "Macintosh; Mac OS 10.0";
		break;
	case QSysInfo::MV_10_1:
		os = "Macintosh; Mac OS 10.1";
		break;
	case QSysInfo::MV_10_2:
		os = "Macintosh; Mac OS 10.2";
		break;
	case QSysInfo::MV_10_3:
		os = "Macintosh; Mac OS 10.3";
		break;
	case QSysInfo::MV_10_4:
		os = "Macintosh; Mac OS 10.4";
		break;
	case QSysInfo::MV_10_5:
		os = "Macintosh; Mac OS 10.5";
		break;
	case QSysInfo::MV_10_6:
		os = "Macintosh; Mac OS 10.6";
		break;
	case QSysInfo::MV_10_7:
		os = "Macintosh; Mac OS 10.7";
		break;
	case QSysInfo::MV_10_8:
		os = "Macintosh; Mac OS 10.8";
		break;
	case QSysInfo::MV_10_9:
		os = "Macintosh; Mac OS 10.9";
		break;
	case QSysInfo::MV_10_10:
		os = "Macintosh; Mac OS 10.10";
		break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QSysInfo::MV_10_11:
		os = "Macintosh; Mac OS 10.11";
		break;
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	case QSysInfo::MV_10_12:
		os = "Macintosh; Mac OS 10.12";
		break;
#endif
	case QSysInfo::MV_Unknown:
		os = "Macintosh; Mac OS unknown";
		break;
	case QSysInfo::MV_IOS_5_0:
		os = "iPhone; iOS 5.0";
		break;
	case QSysInfo::MV_IOS_5_1:
		os = "iPhone; iOS 5.1";
		break;
	case QSysInfo::MV_IOS_6_0:
		os = "iPhone; iOS 6.0";
		break;
	case QSysInfo::MV_IOS_6_1:
		os = "iPhone; iOS 6.1";
		break;
	case QSysInfo::MV_IOS_7_0:
		os = "iPhone; iOS 7.0";
		break;
	case QSysInfo::MV_IOS_7_1:
		os = "iPhone; iOS 7.1";
		break;
	case QSysInfo::MV_IOS_8_0:
		os = "iPhone; iOS 8.0";
		break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QSysInfo::MV_IOS_8_1:
		os = "iPhone; iOS 8.1";
		break;
	case QSysInfo::MV_IOS_8_2:
		os = "iPhone; iOS 8.2";
		break;
	case QSysInfo::MV_IOS_8_3:
		os = "iPhone; iOS 8.3";
		break;
	case QSysInfo::MV_IOS_8_4:
		os = "iPhone; iOS 8.4";
		break;
	case QSysInfo::MV_IOS_9_0:
		os = "iPhone; iOS 9.0";
		break;
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	case QSysInfo::MV_IOS_9_1:
		os = "iPhone; iOS 9.1";
		break;
	case QSysInfo::MV_IOS_9_2:
		os = "iPhone; iOS 9.2";
		break;
	case QSysInfo::MV_IOS_9_3:
		os = "iPhone; iOS 9.3";
		break;
	case QSysInfo::MV_IOS_10_0:
		os = "iPhone; iOS 10.0";
		break;
#endif
	case QSysInfo::MV_IOS:
		os = "iPhone; iOS unknown";
		break;
	default:
		os = "Macintosh";
		break;
	}
	return os;
}
