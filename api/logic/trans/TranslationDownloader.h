#pragma once

#include <QList>
#include <QUrl>
#include <memory>
#include <QObject>
#include <net/NetJob.h>
#include "multimc_logic_export.h"
namespace Net{
class Download;
}
class NetJob;

class MULTIMC_LOGIC_EXPORT TranslationDownloader : public QObject
{
	Q_OBJECT

public:
	TranslationDownloader();

	void downloadTranslations();

private slots:
	void indexRecieved();
	void indexFailed(QString reason);
	void dlFailed(QString reason);
	void dlGood();

private:
	std::shared_ptr<Net::Download> m_index_task;
	NetJobPtr m_dl_job;
	NetJobPtr m_index_job;
	QByteArray m_data;
};
