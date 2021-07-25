#pragma once

#include "BaseExternalTool.h"
#include "QObjectPtr.h"

class BaseInstance;
class SettingsObject;
class LaunchTask;
class QProcess;

class BaseProfiler : public BaseExternalTool
{
    Q_OBJECT
public:
    explicit BaseProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

public
slots:
    void beginProfiling(shared_qobject_ptr<LaunchTask> process);
    void abortProfiling();

protected:
    QProcess *m_profilerProcess;

    virtual void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process) = 0;
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
