#include "WebResourceHandler.h"

#include "net/CacheDownload.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "FileSystem.h"
#include "Env.h"

//FIXME: wrong. needs to be done elsewhere.
QMap<QString, NetJob *> WebResourceHandler::m_activeDownloads;

WebResourceHandler::WebResourceHandler(const QString &url)
	: QObject(), m_url(url)
{
	MetaEntryPtr entry = ENV.metacache()->resolveEntry("icons", url);
	if (!entry->stale)
	{
		setResultFromFile(entry->getFullPath());
	}
	else if (m_activeDownloads.contains(url))
	{
		NetJob *job = m_activeDownloads.value(url);
		connect(job, &NetJob::succeeded, this, &WebResourceHandler::succeeded);
		connect(job, &NetJob::failed, this, [job, this]() {setFailure(job->failReason());});
		connect(job, &NetJob::progress, this, &WebResourceHandler::progress);
	}
	else
	{
		NetJob *job = new NetJob("Icon download");
		job->addNetAction(CacheDownload::make(QUrl(url), entry));
		connect(job, &NetJob::succeeded, this, &WebResourceHandler::succeeded);
		connect(job, &NetJob::failed, this, [job, this]() {setFailure(job->failReason());});
		connect(job, &NetJob::progress, this, &WebResourceHandler::progress);
		connect(job, &NetJob::finished, job, [job](){m_activeDownloads.remove(m_activeDownloads.key(job));job->deleteLater();});
		m_activeDownloads.insert(url, job);
		job->start();
	}
}

void WebResourceHandler::succeeded()
{
	MetaEntryPtr entry = ENV.metacache()->resolveEntry("icons", m_url);
	setResultFromFile(entry->getFullPath());
	m_activeDownloads.remove(m_activeDownloads.key(qobject_cast<NetJob *>(sender())));
}
void WebResourceHandler::progress(qint64 current, qint64 total)
{
	if (total == 0)
	{
		setProgress(101);
	}
	else
	{
		setProgress(current / total);
	}
}

void WebResourceHandler::setResultFromFile(const QString &file)
{
	try
	{
		setResult(FS::read(file));
	}
	catch (Exception &e)
	{
		setFailure(e.cause());
	}
}
