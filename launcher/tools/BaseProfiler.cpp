#include "BaseProfiler.h"
#include "QObjectPtr.h"

#include <QProcess>

BaseProfiler::BaseProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
    : BaseExternalTool(settings, instance, parent)
{
}

void BaseProfiler::beginProfiling(shared_qobject_ptr<LaunchTask> process)
{
    beginProfilingImpl(process);
}

void BaseProfiler::abortProfiling()
{
    abortProfilingImpl();
}

void BaseProfiler::abortProfilingImpl()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilerProcess)
    {
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilerProcess->terminate();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilerProcess->deleteLater();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilerProcess = 0;
    emit abortLaunch(tr("Profiler aborted"));
}

BaseProfiler *BaseProfilerFactory::createProfiler(InstancePtr instance, QObject *parent)
{
    return qobject_cast<BaseProfiler *>(createTool(instance, parent));
}
