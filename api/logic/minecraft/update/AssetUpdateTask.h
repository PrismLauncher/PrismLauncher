#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class MinecraftInstance;

class AssetUpdateTask : public Task
{
	Q_OBJECT
public:
	AssetUpdateTask(MinecraftInstance * inst);
	virtual ~AssetUpdateTask();

	void executeTask() override;

	bool canAbort() const override;

private slots:
	void assetIndexFinished();
	void assetIndexFailed(QString reason);
	void assetsFailed(QString reason);

public slots:
	bool abort() override;

private:
	MinecraftInstance *m_inst;
	NetJobPtr downloadJob;
};
