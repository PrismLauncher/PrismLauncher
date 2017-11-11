#pragma once

#include <cstddef>
#include <QString>
#include <QList>
#include "net/Mode.h"

class ComponentList;

struct RemoteLoadStatus
{
	enum class Type
	{
		Index,
		List,
		Version
	} type = Type::Version;
	size_t componentListIndex = 0;
	bool finished = false;
	bool succeeded = false;
	QString error;
};

struct ComponentUpdateTaskData
{
	ComponentList * m_list = nullptr;
	QList<RemoteLoadStatus> remoteLoadStatusList;
	bool remoteLoadSuccessful = true;
	size_t remoteTasksInProgress = 0;
	ComponentUpdateTask::Mode mode;
	Net::Mode netmode;
};
