#pragma once

#include "BaseExternalTool.h"

class BaseInstance;
class SettingsObject;
class MinecraftProcess;
class QProcess;

class BaseProfiler : public BaseExternalTool
{
	Q_OBJECT
public:
	explicit BaseProfiler(BaseInstance *instance, QObject *parent = 0);

public
slots:
	void beginProfiling(MinecraftProcess *process);
	void abortProfiling();

protected:
	QProcess *m_profilerProcess;

	virtual void beginProfilingImpl(MinecraftProcess *process) = 0;
	virtual void abortProfilingImpl();

signals:
	void readyToLaunch(const QString &message);
	void abortLaunch(const QString &message);
};

class BaseProfilerFactory : public BaseExternalToolFactory
{
public:
	virtual BaseProfiler *createProfiler(BaseInstance *instance, QObject *parent = 0);
};
