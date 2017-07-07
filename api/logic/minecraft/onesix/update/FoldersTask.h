#pragma once

#include "tasks/Task.h"

class OneSixInstance;
class FoldersTask : public Task
{
	Q_OBJECT
public:
	FoldersTask(OneSixInstance * inst);
	void executeTask() override;
private:
	OneSixInstance *m_inst;
};

