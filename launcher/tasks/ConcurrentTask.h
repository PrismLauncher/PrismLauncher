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

    inline auto isMultiStep() const -> bool override { return m_queue.size() > 1; };
    auto getStepProgress() const -> qint64 override;
    auto getStepTotalProgress() const -> qint64 override;

    inline auto getStepStatus() const -> QString override { return m_step_status; }

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
    void setStepStatus(QString status) { m_step_status = status; emit stepStatus(status); };

    virtual void updateState();

protected:
    QString m_name;
    QString m_step_status;

    QQueue<Task::Ptr> m_queue;

    QHash<Task*, Task::Ptr> m_doing;
    QHash<Task*, Task::Ptr> m_done;
    QHash<Task*, Task::Ptr> m_failed;

    int m_total_max_size;
    int m_total_size;

    qint64 m_stepProgress = 0;
    qint64 m_stepTotalProgress = 100;

    bool m_aborted = false;
};
