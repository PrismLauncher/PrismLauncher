#pragma once

#include <QList>
#include <QUrl>
#include <memory>
#include <QObject>

class ByteArrayDownload;
class NetJob;

class TranslationDownloader : public QObject
{
	Q_OBJECT

public:
	TranslationDownloader();

	void downloadTranslations();

private slots:
	void indexRecieved();
	void indexFailed();
	void dlFailed();
	void dlGood();

private:
	std::shared_ptr<ByteArrayDownload> m_index_task;
	std::shared_ptr<NetJob> m_dl_job;
	std::shared_ptr<NetJob> m_index_job;
};