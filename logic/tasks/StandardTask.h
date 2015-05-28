// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Task.h"

#include <QPointer>
#include <memory>

#include "QObjectPtr.h"

class QEventLoop;
class QDir;
class NetAction;
class NetJob;
class INetworkValidator;

class StandardTask : public Task
{
	Q_OBJECT
public:
	explicit StandardTask(QObject *parent = nullptr);

protected:
	// TODO: switch to a future-based system
	void runTask(QObjectPtr<Task> other);
	void runTaskNonBlocking(QObjectPtr<Task> other);
	QByteArray networkGet(const QUrl &url);
	QByteArray networkGetCached(const QString &name, const QString &base, const QString &path, const QUrl &url, const bool alwaysRefetch = false,
								INetworkValidator *validator = nullptr);
	QByteArray networkGetCached(const QString &name, const QString &base, const QString &path, const QUrl &url, const QMap<QString, QString> &headers,
								INetworkValidator *validator = nullptr);
	void networkGetCachedNonBlocking(const QString &name, const QString &base, const QString &path, const QUrl &url, const bool alwaysRefetch = false,
									 INetworkValidator *validator = nullptr);
	void waitOnPending();

private:
	QEventLoop *m_loop;
	QList<QPointer<Task>> m_pendingTasks; // only used to check if the object was deleted
	QList<QObjectPtr<Task>> m_pendingTaskPtrs;

	QObjectPtr<NetJob> wrapDownload(const QString &name, std::shared_ptr<NetAction> action);
};
