#pragma once

#include "BaseExternalTool.h"

class BaseInstance;
class SettingsObject;
class BaseLauncher;
class QProcess;

class BaseProfiler : public BaseExternalTool
{
	Q_OBJECT
public:
	explicit BaseProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

public
slots:
	void beginProfiling(std::shared_ptr<BaseLauncher> process);
	void abortProfiling();

protected:
	QProcess *m_profilerProcess;

	virtual void beginProfilingImpl(std::shared_ptr<BaseLauncher> process) = 0;
	virtual void abortProfilingImpl();

signals:
	void readyToLaunch(const QString &message);
	void abortLaunch(const QString &message);
};

class BaseProfilerFactory : public BaseExternalToolFactory
{
public:
	virtual BaseProfiler *createProfiler(InstancePtr instance, QObject *parent = 0);
};
