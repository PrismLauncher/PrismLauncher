#pragma once

#include "Task.h"
#include "QObjectPtr.h"

#include <QQueue>

class SequentialTask : public Task
{
    Q_OBJECT
public:
    explicit SequentialTask(QObject *parent = nullptr, const QString& task_name = "");
    virtual ~SequentialTask();

    inline auto isMultiStep() const -> bool override { return m_queue.size() > 1; };
    auto getStepProgress() const -> qint64 override;
    auto getStepTotalProgress() const -> qint64 override;

    inline auto getStepStatus() const -> QString override { return m_step_status; }

    void addTask(Task::Ptr task);

protected slots:
    void executeTask() override;
public slots:
    bool abort() override;

private
slots:
    void startNext();
    void subTaskFailed(const QString &msg);
    void subTaskStatus(const QString &msg);
    void subTaskProgress(qint64 current, qint64 total);

protected:
    void setStepStatus(QString status) { m_step_status = status; emit stepStatus(status); };

protected:
    QString m_name;
    QString m_step_status;

    QQueue<Task::Ptr > m_queue;
    int m_currentIndex;

    qint64 m_stepProgress = 0;
    qint64 m_stepTotalProgress = 100;

    bool m_aborted = false;
};
