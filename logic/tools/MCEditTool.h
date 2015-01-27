#pragma once

#include "BaseExternalTool.h"

class MCEditTool : public BaseDetachedTool
{
	Q_OBJECT
public:
	explicit MCEditTool(InstancePtr instance, QObject *parent = 0);

protected:
	QString getSave() const;
	void runImpl() override;
};

class MCEditFactory : public BaseDetachedToolFactory
{
public:
	QString name() const override { return "MCEdit"; }
	void registerSettings(std::shared_ptr<SettingsObject> settings) override;
	BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) override;
	bool check(QString *error) override;
	bool check(const QString &path, QString *error) override;
};
