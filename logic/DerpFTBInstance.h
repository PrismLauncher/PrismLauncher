#pragma once

#include "DerpInstance.h"

class DerpLibrary;

class DerpFTBInstance : public DerpInstance
{
	Q_OBJECT
public:
	explicit DerpFTBInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);
	virtual QString getStatusbarDescription();
	virtual bool menuActionEnabled(QString action_name) const;

	virtual std::shared_ptr<Task> doUpdate(bool only_prepare) override;

	virtual QString id() const;

private:
	std::shared_ptr<DerpLibrary> m_forge;
};
