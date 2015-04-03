#pragma once

#include "logic/OneSixInstance.h"

class OneSixLibrary;

class OneSixFTBInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit OneSixFTBInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);
    virtual ~OneSixFTBInstance(){};

	void copy(const QDir &newDir) override;

	virtual void createProfile();

	virtual QString getStatusbarDescription();

	virtual std::shared_ptr<Task> doUpdate() override;

	virtual QString id() const;

	QDir librariesPath() const override;
	QDir versionsPath() const override;
	bool providesVersionFile() const override;
};
