#pragma once

#include <QQueue>
#include <QSet>

#include "tasks/Task.h"

class ConcurrentTask : public Task {
    Q_OBJECT
public:
    explicit ConcurrentTask(QObject* parent = nullptr, QString task_name = "", int max_concurrent = 6);
    ~ConcurrentTask() override;

    bool canAbort() const override { return true; }

    inline auto isMultiStep() const -> bool override { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.size() > 1; };
    auto getStepProgress() const -> qint64 override;
    auto getStepTotalProgress() const -> qint64 override;

    inline auto getStepStatus() const -> QString override { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_step_status; }

    void addTask(Task::Ptr task);

public slots:
    bool abort() override;

protected
slots:
    void executeTask() override;

    virtual void startNext();

    void subTaskSucceeded(Task::Ptr);
    void subTaskFailed(Task::Ptr, const QString &msg);
    void subTaskStatus(const QString &msg);
    void subTaskProgress(qint64 current, qint64 total);

protected:
    void setStepStatus(QString status) { hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_step_status = status; emit stepStatus(status); };

    virtual void updateState();

protected:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_step_status;

    QQueue<Task::Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue;

    QHash<Task*, Task::Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing;
    QHash<Task*, Task::Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done;
    QHash<Task*, Task::Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed;

    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_max_size;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size;

    qint64 hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepProgress = 0;
    qint64 hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepTotalProgress = 100;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;
};
