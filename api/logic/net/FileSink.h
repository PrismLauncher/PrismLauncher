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
	JobStatus init(QNetworkRequest & request) override;
	JobStatus write(QByteArray & data) override;
	JobStatus abort() override;
	JobStatus finalize(QNetworkReply & reply) override;

protected: /* methods */
	virtual JobStatus initCache(QNetworkRequest &);
	virtual JobStatus finalizeCache(QNetworkReply &reply);

protected: /* data */
	QString m_filename;
	bool wroteAnyData = false;
	std::unique_ptr<QSaveFile> m_output_file;
};
}
