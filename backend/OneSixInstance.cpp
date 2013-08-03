#include "OneSixInstance.h"
#include "OneSixInstance_p.h"

OneSixInstance::OneSixInstance ( const QString& rootDir, SettingsObject* settings, QObject* parent )
: BaseInstance ( new OneSixInstancePrivate(), rootDir, settings, parent )
{

}

GameUpdateTask* OneSixInstance::doUpdate()
{
	return nullptr;
}

MinecraftProcess* OneSixInstance::prepareForLaunch ( QString user, QString session )
{
	return nullptr;
}
