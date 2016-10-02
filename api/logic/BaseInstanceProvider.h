#pragma once

#include <QObject>
#include <QString>
#include "BaseInstance.h"
#include "settings/SettingsObject.h"

#include "multimc_logic_export.h"

using InstanceId = QString;
using InstanceLocator = std::pair<InstancePtr, int>;

enum class InstCreateError
{
	NoCreateError = 0,
	NoSuchVersion,
	UnknownCreateError,
	InstExists,
	CantCreateDir
};

class MULTIMC_LOGIC_EXPORT BaseInstanceProvider : public QObject
{
	Q_OBJECT
public:
	BaseInstanceProvider(SettingsObjectPtr settings) : m_globalSettings(settings)
	{
		// nil
	}
public:
	virtual QList<InstanceId> discoverInstances() = 0;
	virtual InstancePtr loadInstance(const InstanceId &id) = 0;
	virtual void loadGroupList() = 0;
	virtual void saveGroupList() = 0;

	virtual QString getStagedInstancePath()
	{
		return QString();
	}
	virtual bool commitStagedInstance(const QString & keyPath, const QString & path, const QString& instanceName, const QString & groupName)
	{
		return false;
	}
	virtual bool destroyStagingPath(const QString & path)
	{
		return true;
	}

signals:
	// Emit this when the list of provided instances changed
	void instancesChanged();
	// Emit when the set of groups your provider supplies changes.
	void groupsChanged(QSet<QString> groups);

protected:
	SettingsObjectPtr m_globalSettings;
};
