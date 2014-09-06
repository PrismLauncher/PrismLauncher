#pragma once

#include "BaseProfiler.h"

class JProfiler : public BaseProfiler
{
	Q_OBJECT
public:
	JProfiler(InstancePtr instance, QObject *parent = 0);

protected:
	void beginProfilingImpl(MinecraftProcess *process);
};

class JProfilerFactory : public BaseProfilerFactory
{
public:
	QString name() const override { return "JProfiler"; }
	void registerSettings(std::shared_ptr<SettingsObject> settings) override;
	BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) override;
	bool check(QString *error) override;
	bool check(const QString &path, QString *error) override;
};
