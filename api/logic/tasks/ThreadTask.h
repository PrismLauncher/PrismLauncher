#pragma once

#include "Task.h"
#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT ThreadTask : public Task
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