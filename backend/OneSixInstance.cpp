#include "OneSixInstance.h"
#include "OneSixInstance_p.h"
#include "tasks/GameUpdateTask.h"
#include "MinecraftProcess.h"
#include <setting.h>

OneSixInstance::OneSixInstance ( const QString& rootDir, SettingsObject* setting_obj, QObject* parent )
: BaseInstance ( new OneSixInstancePrivate(), rootDir, setting_obj, parent )
{
	I_D(OneSixInstance);
	d->m_settings->registerSetting(new Setting("IntendedVersion", ""));
}

GameUpdateTask* OneSixInstance::doUpdate()
{
	return new GameUpdateTask(this);
}

MinecraftProcess* OneSixInstance::prepareForLaunch ( QString user, QString session )
{
	return nullptr;
}

bool OneSixInstance::setIntendedVersionId ( QString version )
{
	settings().set("IntendedVersion", version);
}

QString OneSixInstance::intendedVersionId()
{
	return settings().get("IntendedVersion").toString();
}
