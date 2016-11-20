#include "sys.h"

#include <sys/utsname.h>

QString Sys::getSystemInfo()
{
	struct utsname buf;
	uname(&buf);
	QString system(buf.sysname);
	QString release(buf.release);

	return system + "; " + release;
}
