#include "NetWorker.h"
#include <QThreadStorage>

NetWorker& NetWorker::spawn()
{
	static QThreadStorage<NetWorker *> storage;
	if (!storage.hasLocalData())
	{
		storage.setLocalData(new NetWorker());
	}
	return *storage.localData();
}
