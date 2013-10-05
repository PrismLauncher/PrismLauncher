#pragma once

#include "Download.h"
#include "HttpMetaCache.h"
#include <QFile>
#include <QTemporaryFile>

class ForgeXzDownload : public Download
{
	Q_OBJECT
public:
	MetaEntryPtr m_entry;
	/// is the saving file already open?
	bool m_opened_for_saving;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QTemporaryFile m_pack200_xz_file;

public:
	explicit ForgeXzDownload(QUrl url, MetaEntryPtr entry);
	
protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();
	
public slots:
	virtual void start();
private:
	void decompressAndInstall();
};

typedef std::shared_ptr<ForgeXzDownload> ForgeXzDownloadPtr;
