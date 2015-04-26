#include "SequentialTask.h"

SequentialTask::SequentialTask(QObject *parent) : Task(parent), m_currentIndex(-1)
{
}

void SequentialTask::addTask(std::shared_ptr<Task> task)
{
	m_queue.append(task);
}

void SequentialTask::executeTask()
{
	m_currentIndex = -1;
	startNext();
}

void SequentialTask::startNext()
{
	if (m_currentIndex != -1)
	{
		std::shared_ptr<Task> previous = m_queue[m_currentIndex];
		disconnect(previous.get(), 0, this, 0);
	}
	m_currentIndex++;
	if (m_queue.isEmpty() || m_currentIndex >= m_queue.size())
	{
		emitSucceeded();
		return;
	}
	std::shared_ptr<Task> next = m_queue[m_currentIndex];
	connect(next.get(), SIGNAL(failed(QString)), this, SLOT(subTaskFailed(QString)));
	connect(next.get(), SIGNAL(status(QString)), this, SLOT(subTaskStatus(QString)));
	connect(next.get(), SIGNAL(progress(qint64, qint64)), this, SLOT(subTaskProgress(qint64, qint64)));
	connect(next.get(), SIGNAL(succeeded()), this, SLOT(startNext()));
	next->start();
}

void SequentialTask::subTaskFailed(const QString &msg)
{
	emitFailed(msg);
}
void SequentialTask::subTaskStatus(const QString &msg)
{
	setStatus(msg);
}
void SequentialTask::subTaskProgress(qint64 current, qint64 total)
{
	if(total == 0)
	{
		setProgress(0);
		return;
	}
	auto dcurrent = (double) current;
	auto dtotal = (double) total;
	auto partial = ((dcurrent / dtotal) * 100.0f)/* / double(m_queue.size())*/;
	// auto bigpartial = double(m_currentIndex) * 100.0f / double(m_queue.size());
	setProgress(partial);
}
