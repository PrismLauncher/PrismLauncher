#pragma once
#include "logic/tasks/Task.h"
#include <QMessageBox>
#include <QNetworkReply>
#include <memory>

class AssetsMigrateTask : public Task
{
	Q_OBJECT
public:
	explicit AssetsMigrateTask(int expected, QObject* parent=0);

protected:
	virtual void executeTask();

private:
	int m_expected;
};
