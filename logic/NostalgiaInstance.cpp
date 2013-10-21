#include "NostalgiaInstance.h"

NostalgiaInstance::NostalgiaInstance ( const QString& rootDir, SettingsObject* settings, QObject* parent )
	: OneSixInstance ( rootDir, settings, parent )
{
	
}

QString NostalgiaInstance::getStatusbarDescription()
{
	return "Nostalgia : " + intendedVersionId();
}

bool NostalgiaInstance::menuActionEnabled(QString action_name) const
{
	return false;
}

/*
ADD MORE
  IF REQUIRED
*/
