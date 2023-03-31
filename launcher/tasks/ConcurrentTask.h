#pragma once

#include <QUuid>
#include <QHash>
#include <QQueue>
#include <QSet>
#include <memory>

#include "tasks/Task.h"

class ConcurrentTask : public Task {
    Q_OBJECT
public:
    using Ptr = shared_qobject_ptr<ConcurrentTask>;

    explicit ConcurrentTask(QObject* parent = nullptr, QString task_name = "", int max_concurrent = 6);
    ~ConcurrentTask() override;

    bool canAbort() const override { return true; }

    inline auto isMultiStep() const -> bool override { return totalSize() > 1; };
    auto getStepProgress() const -> TaskStepProgressList override;

    void addTask(Task::Ptr task);

public slots:
    bool abort() override;

    /** Resets the internal state of the task.
     *  This allows the same task to be re-used.
     */
    void clear();

protected
slots:
    void executeTask() override;

    virtual void startNext();

    void subTaskSucceeded(Task::Ptr);
    void subTaskFailed(Task::Ptr, const QString &msg);
    void subTaskStatus(Task::Ptr task, const QString &msg);
    void subTaskProgress(Task::Ptr task, qint64 current, qint64 total);
    void subTaskStepProgress(Task::Ptr task, TaskStepProgressList task_step_progress);

protected:
    // NOTE: This is not thread-safe.
    [[nodiscard]] unsigned int totalSize() const { return m_queue.size() + m_doing.size() + m_done.size(); }

    void updateStepProgress();

    virtual void updateState();

protected:
    QString m_name;
    QString m_step_status;

    QQueue<Task::Ptr> m_queue;

    QHash<Task*, Task::Ptr> m_doing; 
    QHash<Task*, Task::Ptr> m_done;
    QHash<Task*, Task::Ptr> m_failed;
    QHash<Task*, Task::Ptr> m_succeeded;

    QHash<QUuid, std::shared_ptr<TaskStepProgress>> m_task_progress;

    int m_total_max_size;

    qint64 m_stepProgress = 0;
    qint64 m_stepTotalProgress = 100;

    bool m_aborted = false;
};
