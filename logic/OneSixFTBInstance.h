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

	virtual std::shared_ptr<Task> doUpdate() override;

	virtual QString id() const;

	virtual QDir librariesPath() const override;
	virtual QDir versionsPath() const override;

private:
	std::shared_ptr<OneSixLibrary> m_forge;
};
