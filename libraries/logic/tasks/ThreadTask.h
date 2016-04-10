#pragma once

#include "Task.h"

class ThreadTask : public Task
{
	Q_OBJECT
public:
	explicit ThreadTask(Task * internal, QObject * parent = nullptr);

protected:
	void executeTask() {};

public slots:
	virtual void start();

private slots:
	void iternal_started();
	void iternal_progress(qint64 current, qint64 total);
	void iternal_succeeded();
	void iternal_failed(QString reason);
	void iternal_status(QString status);
private:
	Task * m_internal;
};