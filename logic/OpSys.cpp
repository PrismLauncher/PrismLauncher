#include "OpSys.h"

OpSys OpSys_fromString(QString name)
{
	if(name == "linux")
		return Os_Linux;
	if(name == "windows")
		return Os_Windows;
	if(name == "osx")
		return Os_OSX;
	return Os_Other;
}