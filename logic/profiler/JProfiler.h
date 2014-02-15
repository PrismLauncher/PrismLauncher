#pragma once

#include "BaseProfiler.h"

class JProfiler : public BaseProfiler
{
	Q_OBJECT
public:
	JProfiler(OneSixInstance *instance, QObject *parent = 0);

protected:
	void beginProfilingImpl(MinecraftProcess *process);
};

class JProfilerFactory : public BaseProfilerFactory
{
public:
	void registerSettings(SettingsObject *settings) override;
	BaseProfiler *createProfiler(OneSixInstance *instance, QObject *parent = 0) override;
	bool check(const QString &path, QString *error) override;
};
