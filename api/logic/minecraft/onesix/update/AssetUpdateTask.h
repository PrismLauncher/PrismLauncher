#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class OneSixInstance;

class AssetUpdateTask : public Task
{
public:
	AssetUpdateTask(OneSixInstance * inst);
	void executeTask() override;

	bool canAbort() const override;

private slots:
	void assetIndexFinished();
	void assetIndexFailed(QString reason);
	void assetsFailed(QString reason);

public slots:
	bool abort() override;

private:
	OneSixInstance *m_inst;
	NetJobPtr downloadJob;
};
