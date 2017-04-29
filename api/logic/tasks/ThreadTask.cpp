#include "ThreadTask.h"
#include <QtConcurrentRun>
ThreadTask::ThreadTask(Task * internal, QObject *parent) : Task(parent), m_internal(internal)
{
}

void ThreadTask::start()
{
	connect(m_internal, SIGNAL(failed(QString)), SLOT(iternal_failed(QString)));
	connect(m_internal, SIGNAL(progress(qint64,qint64)), SLOT(iternal_progress(qint64,qint64)));
	connect(m_internal, SIGNAL(started()), SLOT(iternal_started()));
	connect(m_internal, SIGNAL(status(QString)), SLOT(iternal_status(QString)));
	connect(m_internal, SIGNAL(succeeded()), SLOT(iternal_succeeded()));
	m_running = true;
	QtConcurrent::run(m_internal, &Task::start);
}

void ThreadTask::iternal_failed(QString reason)
{
	emitFailed(reason);
}

void ThreadTask::iternal_progress(qint64 current, qint64 total)
{
	progress(current, total);
}

void ThreadTask::iternal_started()
{
	emit started();
}

void ThreadTask::iternal_status(QString status)
{
	setStatusText(status);
}

void ThreadTask::iternal_succeeded()
{
	emitSucceeded();
}
