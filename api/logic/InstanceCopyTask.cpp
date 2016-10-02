#include "InstanceCopyTask.h"
#include "BaseInstanceProvider.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"

InstanceCopyTask::InstanceCopyTask(SettingsObjectPtr settings, BaseInstanceProvider* target, InstancePtr origInstance, const QString& instName, const QString& instIcon, const QString& instGroup, bool copySaves)
{
	m_globalSettings = settings;
	m_target = target;
	m_origInstance = origInstance;
	m_instName = instName;
	m_instIcon = instIcon;
	m_instGroup = instGroup;
	m_copySaves = copySaves;
}

void InstanceCopyTask::executeTask()
{
	setStatus(tr("Copying instance %1").arg(m_origInstance->name()));
	std::unique_ptr<IPathMatcher> matcher;
	if(!m_copySaves)
	{
		// FIXME: get this from the original instance type...
		auto matcherReal = new RegexpMatcher("[.]?minecraft/saves");
		matcherReal->caseSensitive(false);
		matcher.reset(matcherReal);
	}

	QString stagingPath = m_target->getStagedInstancePath();
	FS::copy folderCopy(m_origInstance->instanceRoot(), stagingPath);
	folderCopy.followSymlinks(false).blacklist(matcher.get());
	if (!folderCopy())
	{
		m_target->destroyStagingPath(stagingPath);
		emitFailed(tr("Instance folder copy failed."));
		return;
	}

	// FIXME: shouldn't this be able to report errors?
	auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(stagingPath, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");

	// FIXME: and this too? errors???
	m_origInstance->copy(stagingPath);

	InstancePtr inst(new NullInstance(m_globalSettings, instanceSettings, stagingPath));
	inst->setName(m_instName);
	inst->setIconKey(m_instIcon);
	m_target->commitStagedInstance(stagingPath, stagingPath, m_instName, m_instGroup);
	emitSucceeded();
}
