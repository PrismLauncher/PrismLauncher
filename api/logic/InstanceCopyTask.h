#pragma once

#include "tasks/Task.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include <QUrl>
#include "settings/SettingsObject.h"
#include "BaseVersion.h"
#include "BaseInstance.h"

class BaseInstanceProvider;

class MULTIMC_LOGIC_EXPORT InstanceCopyTask : public Task
{
	Q_OBJECT
public:
	explicit InstanceCopyTask(SettingsObjectPtr settings, BaseInstanceProvider * target, InstancePtr origInstance, const QString &instName,
		const QString &instIcon, const QString &instGroup, bool copySaves);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;

private: /* data */
	SettingsObjectPtr m_globalSettings;
	BaseInstanceProvider * m_target = nullptr;
	InstancePtr m_origInstance;
	QString m_instName;
	QString m_instIcon;
	QString m_instGroup;
	bool m_copySaves = false;
};


