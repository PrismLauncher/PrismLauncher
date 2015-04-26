#pragma once

#include "Task.h"

#include <QQueue>
#include <memory>

class SequentialTask : public Task
{
	Q_OBJECT
public:
	explicit SequentialTask(QObject *parent = 0);

	void addTask(std::shared_ptr<Task> task);

protected:
	void executeTask();

private
slots:
	void startNext();
	void subTaskFailed(const QString &msg);
	void subTaskStatus(const QString &msg);
	void subTaskProgress(qint64 current, qint64 total);

private:
	QQueue<std::shared_ptr<Task> > m_queue;
	int m_currentIndex;
};
