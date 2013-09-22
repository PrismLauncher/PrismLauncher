#pragma once
#include <QString>
enum OpSys
{
	Os_Windows,
	Os_Linux,
	Os_OSX,
	Os_Other
};

OpSys OpSys_fromString(QString);
QString OpSys_toString(OpSys);

#ifdef Q_OS_WIN32
	#define currentSystem Os_Windows
#else
	#ifdef Q_OS_MAC
		#define currentSystem Os_OSX
	#else
		#define currentSystem Os_Linux
	#endif
#endif