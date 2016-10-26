#include "InstanceCopyTask.h"
#include "BaseInstanceProvider.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

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

	m_stagingPath = m_target->getStagedInstancePath();
	FS::copy folderCopy(m_origInstance->instanceRoot(), m_stagingPath);
	folderCopy.followSymlinks(false).blacklist(matcher.get());

	m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &InstanceCopyTask::copyFinished);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &InstanceCopyTask::copyAborted);
	m_copyFutureWatcher.setFuture(m_copyFuture);
}

void InstanceCopyTask::copyFinished()
{
	auto successful = m_copyFuture.result();
	if(!successful)
	{
		m_target->destroyStagingPath(m_stagingPath);
		emitFailed(tr("Instance folder copy failed."));
		return;
	}
	// FIXME: shouldn't this be able to report errors?
	auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");

	// FIXME: and this too? errors???
	m_origInstance->copy(m_stagingPath);

	InstancePtr inst(new NullInstance(m_globalSettings, instanceSettings, m_stagingPath));
	inst->setName(m_instName);
	inst->setIconKey(m_instIcon);
	m_target->commitStagedInstance(m_stagingPath, m_stagingPath, m_instName, m_instGroup);
	emitSucceeded();
}

void InstanceCopyTask::copyAborted()
{
	m_target->destroyStagingPath(m_stagingPath);
	emitFailed(tr("Instance folder copy has been aborted."));
	return;
}
