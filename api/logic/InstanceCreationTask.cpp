#include "InstanceCreationTask.h"
#include "BaseInstanceProvider.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"

//FIXME: remove this
#include "minecraft/MinecraftInstance.h"

InstanceCreationTask::InstanceCreationTask(SettingsObjectPtr settings, const QString & stagingPath, BaseVersionPtr version,
	const QString& instName, const QString& instIcon, const QString& instGroup)
{
	m_globalSettings = settings;
	m_stagingPath = stagingPath;
	m_instName = instName;
	m_instIcon = instIcon;
	m_instGroup = instGroup;
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
		auto inst = new MinecraftInstance(m_globalSettings, instanceSettings, m_stagingPath);
		inst->setComponentVersion("net.minecraft", m_version->descriptor());
		inst->setName(m_instName);
		inst->setIconKey(m_instIcon);
		inst->init();
		instanceSettings->resumeSave();
	}
	emitSucceeded();
}
