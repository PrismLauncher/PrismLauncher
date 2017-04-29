#pragma once
#include "Sink.h"
#include <QSaveFile>

namespace Net {
class FileSink : public Sink
{
public: /* con/des */
	FileSink(QString filename);
	virtual ~FileSink();

public: /* methods */
	Task::Status init(QNetworkRequest & request) override;
	Task::Status write(QByteArray & data) override;
	Task::Status abort() override;
	Task::Status finalize(QNetworkReply & reply) override;
	bool hasLocalData() override;

protected: /* methods */
	virtual Task::Status initCache(QNetworkRequest &);
	virtual Task::Status finalizeCache(QNetworkReply &reply);

protected: /* data */
	QString m_filename;
	bool wroteAnyData = false;
	std::unique_ptr<QSaveFile> m_output_file;
};
}
