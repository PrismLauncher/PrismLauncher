#include "InstanceCreationTask.h"
#include "BaseInstanceProvider.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"

//FIXME: remove this
#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"

InstanceCreationTask::InstanceCreationTask(BaseVersionPtr version)
{
	m_version = version;
}

void InstanceCreationTask::executeTask()
{
	setStatus(tr("Creating instance from version %1").arg(m_version->name()));
	{
		auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg"));
		instanceSettings->suspendSave();
		instanceSettings->registerSetting("InstanceType", "Legacy");
		instanceSettings->set("InstanceType", "OneSix");
		MinecraftInstance inst(m_globalSettings, instanceSettings, m_stagingPath);
		auto components = inst.getComponentList();
		components->buildingFromScratch();
		components->setComponentVersion("net.minecraft", m_version->descriptor(), true);
		inst.setName(m_instName);
		inst.setIconKey(m_instIcon);
		inst.init();
		instanceSettings->resumeSave();
	}
	emitSucceeded();
}
