#include "TranslationDownloader.h"
#include "logic/net/NetJob.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/CacheDownload.h"
#include "logic/net/URLConstants.h"
#include "MultiMC.h"

TranslationDownloader::TranslationDownloader()
{
}
void TranslationDownloader::downloadTranslations()
{
	QLOG_DEBUG() << "Downloading Translations Index...";
	m_index_job.reset(new NetJob("Translations Index"));
	m_index_task = ByteArrayDownload::make(QUrl("http://files.multimc.org/translations/index"));
	m_index_job->addNetAction(m_index_task);
	connect(m_index_job.get(), &NetJob::failed, this, &TranslationDownloader::indexFailed);
	connect(m_index_job.get(), &NetJob::succeeded, this, &TranslationDownloader::indexRecieved);
	m_index_job->start();
}
void TranslationDownloader::indexRecieved()
{
	QLOG_DEBUG() << "Got translations index!";
	m_dl_job.reset(new NetJob("Translations"));
	QList<QByteArray> lines = m_index_task->m_data.split('\n');
	for (const auto line : lines)
	{
		if (!line.isEmpty())
		{
			CacheDownloadPtr dl = CacheDownload::make(
				QUrl(URLConstants::TRANSLATIONS_BASE_URL + line),
				MMC->metacache()->resolveEntry("translations", "mmc_" + line));
			m_dl_job->addNetAction(dl);
		}
	}
	connect(m_dl_job.get(), &NetJob::succeeded, this, &TranslationDownloader::dlGood);
	connect(m_dl_job.get(), &NetJob::failed, this, &TranslationDownloader::dlFailed);
	m_dl_job->start();
}
void TranslationDownloader::dlFailed()
{
	QLOG_ERROR() << "Translations Download Failed!";
}
void TranslationDownloader::dlGood()
{
	QLOG_DEBUG() << "Got translations!";
}
void TranslationDownloader::indexFailed()
{
	QLOG_ERROR() << "Translations Index Download Failed!";
}
