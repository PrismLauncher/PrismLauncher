#pragma once

#include "BaseProfiler.h"

class JProfiler : public BaseProfiler
{
	Q_OBJECT
public:
	JProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

protected:
	void beginProfilingImpl(BaseProcess *process);
};

class JProfilerFactory : public BaseProfilerFactory
{
public:
	QString name() const override { return "JProfiler"; }
	void registerSettings(SettingsObjectPtr settings) override;
	BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) override;
	bool check(QString *error) override;
	bool check(const QString &path, QString *error) override;
};
