#include <QtCore>
#include "fullversion.h"
#include <library.h>

QList<QSharedPointer<Library> > FullVersion::getActiveNormalLibs()
{
	QList<QSharedPointer<Library> > output;
	for ( auto lib: libraries )
	{
		if (lib->getIsActive() && !lib->getIsNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<QSharedPointer<Library> > FullVersion::getActiveNativeLibs()
{
	QList<QSharedPointer<Library> > output;
	for ( auto lib: libraries )
	{
		if (lib->getIsActive() && lib->getIsNative())
		{
			output.append(lib);
		}
	}
	return output;
}
