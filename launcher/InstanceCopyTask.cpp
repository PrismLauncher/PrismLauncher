#include "InstanceCopyTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

InstanceCopyTask::InstanceCopyTask(InstancePtr origInstance, const InstanceCopyPrefs& prefs)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_origInstance = origInstance;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_keepPlaytime = prefs.isKeepPlaytimeEnabled();

    QString filters = prefs.getSelectedFiltersAsRegex();
    if (!filters.isEmpty())
    {
        // Set regex filter:
        // FIXME: get this from the original instance type...
        auto matcherReal = new RegexpMatcher(filters);
        matcherReal->caseSensitive(false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher.reset(matcherReal);
    }
}

void InstanceCopyTask::executeTask()
{
    setStatus(tr("Copying instance %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_origInstance->name()));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this]{
        FS::copy folderCopy(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_origInstance->instanceRoot(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);
        folderCopy.followSymlinks(false).matcher(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher.get());

        return folderCopy();
    });
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &InstanceCopyTask::copyFinished);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &InstanceCopyTask::copyAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture);
}

void InstanceCopyTask::copyFinished()
{
    auto successful = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.result();
    if(!successful)
    {
        emitFailed(tr("Instance folder copy failed."));
        return;
    }
    // FIXME: shouldn't this be able to report errors?
    auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, "instance.cfg"));

    InstancePtr inst(new NullInstance(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings, instanceSettings, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath));
    inst->setName(name());
    inst->setIconKey(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon);
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_keepPlaytime) {
        inst->resetTimePlayed();
    }
    emitSucceeded();
}

void InstanceCopyTask::copyAborted()
{
    emitFailed(tr("Instance folder copy has been aborted."));
    return;
}
