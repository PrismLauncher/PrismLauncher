// Licensed under the Apache-2.0 license. See README.md for details.

#include "StandardTask.h"

#include <QEventLoop>
#include <QProcess>

#include "net/CacheDownload.h"
#include "net/ByteArrayDownload.h"
#include "net/NetJob.h"
#include "FileSystem.h"
#include "Exception.h"
#include "Env.h"

StandardTask::StandardTask(QObject *parent)
	: Task(parent)
{
	m_loop = new QEventLoop(this);
}

void StandardTask::runTask(QObjectPtr<Task> other)
{
	connect(other.get(), &Task::succeeded, m_loop, &QEventLoop::quit);
	connect(other.get(), &Task::failed, m_loop, &QEventLoop::quit);
	connect(other.get(), &Task::progress, this, [this](qint64 current, qint64 total){setProgress(current / total);});
	connect(other.get(), &Task::status, this, &StandardTask::setStatus);
	if (!other->isRunning())
	{
		other->start();
	}
	if (other->isRunning())
	{
		m_loop->exec();
	}
	disconnect(other.get(), 0, m_loop, 0);
	disconnect(other.get(), 0, this, 0);
	other->deleteLater();
	if (!other->successful())
	{
		throw Exception(other->failReason());
	}
}
void StandardTask::runTaskNonBlocking(QObjectPtr<Task> other)
{
	if (!other)
	{
		return;
	}
	m_pendingTasks.append(other.get());
	m_pendingTaskPtrs.append(other);
	other->start();
}
QByteArray StandardTask::networkGet(const QUrl &url)
{
	ByteArrayDownloadPtr task = ByteArrayDownload::make(url);
	runTask(wrapDownload("", task));
	return task->m_data;
}
QByteArray StandardTask::networkGetCached(const QString &name, const QString &base, const QString &path, const QUrl &url, const bool alwaysRefetch,
										  INetworkValidator *validator)
{
	MetaEntryPtr entry = ENV.metacache()->resolveEntry(base, path);
	if (!alwaysRefetch && !entry->stale)
	{
		if (validator) { delete validator; }
		return FS::read(entry->getFullPath());
	}
	else if (alwaysRefetch)
	{
		entry->stale = true;
	}
	CacheDownloadPtr task = CacheDownload::make(url, entry);
	task->setValidator(validator);
	runTask(wrapDownload(name, task));
	return FS::read(entry->getFullPath());
}
QByteArray StandardTask::networkGetCached(const QString &name, const QString &base, const QString &path, const QUrl &url, const QMap<QString, QString> &headers,
										  INetworkValidator *validator)
{
	MetaEntryPtr entry = ENV.metacache()->resolveEntry(base, path);
	if (!entry->stale)
	{
		if (validator) { delete validator; }
		return FS::read(entry->getFullPath());
	}
	CacheDownloadPtr task = CacheDownload::make(url, entry);
	//task->setHeaders(headers);
	task->setValidator(validator);
	runTask(wrapDownload(name, task));
	return FS::read(entry->getFullPath());
}
void StandardTask::networkGetCachedNonBlocking(const QString &name, const QString &base, const QString &path, const QUrl &url, const bool alwaysRefetch,
											   INetworkValidator *validator)
{
	MetaEntryPtr entry = ENV.metacache()->resolveEntry(base, path);
	if (!alwaysRefetch && !entry->stale)
	{
		return;
	}
	CacheDownloadPtr dl = CacheDownload::make(url, entry);
	dl->setValidator(validator);
	runTaskNonBlocking(wrapDownload(name, dl));
}
void StandardTask::waitOnPending()
{
	for (int i = 0; i < m_pendingTasks.size(); ++i)
	{
		if (m_pendingTasks.at(i) && m_pendingTasks.at(i)->isRunning())
		{
			runTask(m_pendingTaskPtrs.at(i));
		}
	}
}

QObjectPtr<NetJob> StandardTask::wrapDownload(const QString &name, std::shared_ptr<NetAction> action)
{
	NetJobPtr task = NetJobPtr(new NetJob(name));
	task->addNetAction(action);
	return task;
}
