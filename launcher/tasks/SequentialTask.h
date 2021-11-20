#pragma once

#include "Task.h"
#include "QObjectPtr.h"

#include <QQueue>

class SequentialTask : public Task
{
    Q_OBJECT
public:
    explicit SequentialTask(QObject *parent = 0);
    virtual ~SequentialTask() {};

    void addTask(shared_qobject_ptr<Task> task);

protected:
    void executeTask();

private
slots:
    void startNext();
    void subTaskFailed(const QString &msg);
    void subTaskStatus(const QString &msg);
    void subTaskProgress(qint64 current, qint64 total);

private:
    QQueue<shared_qobject_ptr<Task> > m_queue;
    int m_currentIndex;
};
