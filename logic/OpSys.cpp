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

QString OpSys_toString(OpSys name)
{
	switch(name)
	{
		case Os_Linux: return "linux";
		case Os_OSX: return "osx";
		case Os_Windows: return "windows";
		default: return "other";
	}
}