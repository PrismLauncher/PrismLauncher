#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QUrl>
#include "BaseInstance.h"
#include "BaseVersion.h"
#include "InstanceCopyPrefs.h"
#include "InstanceTask.h"
#include "net/NetJob.h"
#include "settings/SettingsObject.h"
#include "tasks/Task.h"

class InstanceCopyTask : public InstanceTask
{
    Q_OBJECT
public:
    explicit InstanceCopyTask(InstancePtr origInstance, const InstanceCopyPrefs& prefs);

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;
    void copyFinished();
    void copyAborted();

private:
    /* data */
    InstancePtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_origInstance;
    QFuture<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture;
    QFutureWatcher<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher;
    std::unique_ptr<IPathMatcher> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_keepPlaytime;
};
