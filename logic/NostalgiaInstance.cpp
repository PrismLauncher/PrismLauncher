#include "NostalgiaInstance.h"

NostalgiaInstance::NostalgiaInstance ( const QString& rootDir, SettingsObject* settings, QObject* parent )
	: OneSixInstance ( rootDir, settings, parent )
{
	
}

QString NostalgiaInstance::getStatusbarDescription()
{
	return "Nostalgia : " + intendedVersionId();
}


/*
ADD MORE
  IF REQUIRED
*/
