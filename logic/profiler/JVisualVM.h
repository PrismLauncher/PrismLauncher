#pragma once

#include "BaseProfiler.h"

class JVisualVM : public BaseProfiler
{
	Q_OBJECT
public:
	JVisualVM(BaseInstance *instance, QObject *parent = 0);

protected:
	void beginProfilingImpl(MinecraftProcess *process);
};

class JVisualVMFactory : public BaseProfilerFactory
{
public:
	QString name() const override { return "JVisualVM"; }
	void registerSettings(SettingsObject *settings) override;
	BaseExternalTool *createTool(BaseInstance *instance, QObject *parent = 0) override;
	bool check(QString *error) override;
	bool check(const QString &path, QString *error) override;
};
