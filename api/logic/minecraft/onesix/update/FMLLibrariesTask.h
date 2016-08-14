#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class OneSixInstance;

class FMLLibrariesTask : public Task
{
public:
	FMLLibrariesTask(OneSixInstance * inst);

	void executeTask() override;

	bool canAbort() const override;

private slots:
	void fmllibsFinished();
	void fmllibsFailed(QString reason);

public slots:
	bool abort() override;

private:
	OneSixInstance *m_inst;
	NetJobPtr downloadJob;
	QList<FMLlib> fmlLibsToProcess;
};

