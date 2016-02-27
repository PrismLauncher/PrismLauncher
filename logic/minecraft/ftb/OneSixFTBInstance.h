#pragma once

#include "minecraft/onesix/OneSixInstance.h"

class OneSixFTBInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit OneSixFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
    virtual ~OneSixFTBInstance(){};

	void copy(const QDir &newDir) override;

	virtual void createProfile() override;

	virtual std::shared_ptr<Task> createUpdateTask() override;

	virtual QString id() const override;

	QDir librariesPath() const override;
	QDir versionsPath() const override;
	bool providesVersionFile() const override;
	virtual QString typeName() const override;

private:
	SettingsObjectPtr m_globalSettings;
};
