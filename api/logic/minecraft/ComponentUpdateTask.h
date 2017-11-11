#pragma once

#include "tasks/Task.h"
#include "net/Mode.h"

#include <memory>
class ComponentList;
struct ComponentUpdateTaskData;

class ComponentUpdateTask : public Task
{
	Q_OBJECT
public:
	enum class Mode
	{
		Launch,
		Resolution
	};

public:
	explicit ComponentUpdateTask(Mode mode, Net::Mode netmode, ComponentList * list, QObject *parent = 0);
	virtual ~ComponentUpdateTask();

protected:
	void executeTask();

private:
	void loadComponents();
	void resolveDependencies(bool checkOnly);

	void remoteLoadSucceeded(size_t index);
	void remoteLoadFailed(size_t index, const QString &msg);
	void checkIfAllFinished();

private:
	std::unique_ptr<ComponentUpdateTaskData> d;
};
