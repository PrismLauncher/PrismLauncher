#pragma once

#include "OneSixInstance.h"

class OneSixLibrary;

class OneSixFTBInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit OneSixFTBInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);
	virtual QString getStatusbarDescription();
	virtual bool menuActionEnabled(QString action_name) const;

	virtual std::shared_ptr<Task> doUpdate(bool only_prepare) override;

private:
	std::shared_ptr<OneSixLibrary> m_forge;
};
