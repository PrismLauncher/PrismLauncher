#pragma once

#include "logic/minecraft/OneSixInstance.h"

class OneSixLibrary;

class OneSixFTBInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit OneSixFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
    virtual ~OneSixFTBInstance(){};

	void copy(const QDir &newDir) override;

	virtual void createProfile();

	virtual QString getStatusbarDescription();

	virtual std::shared_ptr<Task> doUpdate() override;

	virtual QString id() const;

	QDir librariesPath() const override;
	QDir versionsPath() const override;
	bool providesVersionFile() const override;
private:
	SettingsObjectPtr m_globalSettings;
};
