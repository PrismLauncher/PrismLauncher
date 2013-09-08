#pragma once

#include "Download.h"
#include "HttpMetaCache.h"
#include <QFile>
#include <qcryptographichash.h>

class CacheDownload : public Download
{
	Q_OBJECT
public:
	MetaEntryPtr m_entry;
	/// is the saving file already open?
	bool m_opened_for_saving;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;
	/// the hash-as-you-download
	QCryptographicHash md5sum;
public:
	explicit CacheDownload(QUrl url, MetaEntryPtr entry);
	
protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();
	
public slots:
	virtual void start();
};

typedef QSharedPointer<CacheDownload> CacheDownloadPtr;
