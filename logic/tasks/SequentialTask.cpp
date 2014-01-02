#include "SequentialTask.h"

SequentialTask::SequentialTask(QObject *parent) :
	Task(parent), m_currentIndex(-1)
{

}

QString SequentialTask::getStatus() const
{
	if (m_queue.isEmpty() || m_currentIndex >= m_queue.size())
	{
		return QString();
	}
	return m_queue.at(m_currentIndex)->getStatus();
}

void SequentialTask::getProgress(qint64 &current, qint64 &total)
{
	current = 0;
	total = 0;
	for (int i = 0; i < m_queue.size(); ++i)
	{
		qint64 subCurrent, subTotal;
		m_queue.at(i)->getProgress(subCurrent, subTotal);
		current += subCurrent;
		total += subTotal;
	}
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
	connect(next.get(), SIGNAL(progress(qint64,qint64)), this, SLOT(subTaskProgress()));
	connect(next.get(), SIGNAL(succeeded()), this, SLOT(startNext()));
	next->start();
	emit status(getStatus());
}

void SequentialTask::subTaskFailed(const QString &msg)
{
	emitFailed(msg);
}
void SequentialTask::subTaskStatus(const QString &msg)
{
	setStatus(msg);
}
void SequentialTask::subTaskProgress()
{
	qint64 current, total;
	getProgress(current, total);
	setProgress(100 * current / total);
}
