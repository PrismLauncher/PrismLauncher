#include "NetWorker.h"
#include <QThreadStorage>

class NetWorker::Private
{
public:
	QNetworkAccessManager manager;
};

NetWorker::NetWorker ( QObject* parent ) : QObject ( parent )
{
	d = new Private();
}

QNetworkAccessManager& NetWorker::qnam()
{
	auto & w = worker();
	return w.d->manager;
}


NetWorker& NetWorker::worker()
{
	static QThreadStorage<NetWorker *> storage;
	if (!storage.hasLocalData())
	{
		storage.setLocalData(new NetWorker());
	}
	return *storage.localData();
}
