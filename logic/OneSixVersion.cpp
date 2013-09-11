#include "OneSixVersion.h"
#include "OneSixLibrary.h"

QList<QSharedPointer<OneSixLibrary> > OneSixVersion::getActiveNormalLibs()
{
	QList<QSharedPointer<OneSixLibrary> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<QSharedPointer<OneSixLibrary> > OneSixVersion::getActiveNativeLibs()
{
	QList<QSharedPointer<OneSixLibrary> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}


