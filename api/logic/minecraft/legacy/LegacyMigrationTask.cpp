#include "LegacyMigrationTask.h"
#include "BaseInstanceProvider.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

LegacyMigrationTask::LegacyMigrationTask(SettingsObjectPtr settings, const QString & stagingPath, InstancePtr origInstance)
{
	m_globalSettings = settings;
	m_stagingPath = stagingPath;
	m_origInstance = origInstance;
}

void LegacyMigrationTask::executeTask()
{
	setStatus(tr("Copying instance %1").arg(m_origInstance->name()));

	FS::copy folderCopy(m_origInstance->instanceRoot(), m_stagingPath);
	folderCopy.followSymlinks(true);

	m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &LegacyMigrationTask::copyFinished);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &LegacyMigrationTask::copyAborted);
	m_copyFutureWatcher.setFuture(m_copyFuture);
}

void LegacyMigrationTask::copyFinished()
{
	auto successful = m_copyFuture.result();
	if(!successful)
	{
		emitFailed(tr("Instance folder copy failed."));
		return;
	}
	// FIXME: shouldn't this be able to report errors?
	auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");

	InstancePtr inst(new NullInstance(m_globalSettings, instanceSettings, m_stagingPath));
	inst->setName(tr("%1 (Migrated)").arg(m_origInstance->name()));
	emitSucceeded();
}

void LegacyMigrationTask::copyAborted()
{
	emitFailed(tr("Instance folder copy has been aborted."));
	return;
}

