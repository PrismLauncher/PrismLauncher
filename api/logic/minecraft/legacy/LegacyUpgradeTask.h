#pragma once

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "BaseVersion.h"
#include "BaseInstance.h"


class BaseInstanceProvider;

class MULTIMC_LOGIC_EXPORT LegacyUpgradeTask : public InstanceTask
{
	Q_OBJECT
public:
	explicit LegacyUpgradeTask(InstancePtr origInstance);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;
	void copyFinished();
	void copyAborted();

private: /* data */
	InstancePtr m_origInstance;
	QFuture<bool> m_copyFuture;
	QFutureWatcher<bool> m_copyFutureWatcher;
};
