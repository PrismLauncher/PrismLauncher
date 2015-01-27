#pragma once

#include "BaseProfiler.h"

class JVisualVM : public BaseProfiler
{
	Q_OBJECT
public:
	JVisualVM(InstancePtr instance, QObject *parent = 0);

protected:
	void beginProfilingImpl(BaseProcess *process);
};

class JVisualVMFactory : public BaseProfilerFactory
{
public:
	QString name() const override { return "JVisualVM"; }
	void registerSettings(std::shared_ptr<SettingsObject> settings) override;
	BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) override;
	bool check(QString *error) override;
	bool check(const QString &path, QString *error) override;
};
