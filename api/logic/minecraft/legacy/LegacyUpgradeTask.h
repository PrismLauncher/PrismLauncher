#pragma once

#include "tasks/Task.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "BaseVersion.h"
#include "BaseInstance.h"


class BaseInstanceProvider;

class MULTIMC_LOGIC_EXPORT LegacyUpgradeTask : public Task
{
	Q_OBJECT
public:
	explicit LegacyUpgradeTask(SettingsObjectPtr settings, const QString & stagingPath, InstancePtr origInstance, const QString & newName);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;
	void copyFinished();
	void copyAborted();

private: /* data */
	SettingsObjectPtr m_globalSettings;
	InstancePtr m_origInstance;
	QString m_stagingPath;
	QString m_newName;
	QFuture<bool> m_copyFuture;
	QFutureWatcher<bool> m_copyFutureWatcher;
};



