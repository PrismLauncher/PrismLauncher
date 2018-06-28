#pragma once

#include "Task.h"

#include <QQueue>
#include <memory>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT SequentialTask : public Task
{
	Q_OBJECT
public:
	explicit SequentialTask(QObject *parent = 0);
	virtual ~SequentialTask() {};

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
