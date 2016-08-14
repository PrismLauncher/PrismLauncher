#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class OneSixInstance;

class LibrariesTask : public Task
{
public:
	LibrariesTask(OneSixInstance * inst);

	void executeTask() override;

	bool canAbort() const override;

private slots:
	void jarlibFailed(QString reason);

public slots:
	bool abort() override;

private:
	OneSixInstance *m_inst;
	NetJobPtr downloadJob;
};
