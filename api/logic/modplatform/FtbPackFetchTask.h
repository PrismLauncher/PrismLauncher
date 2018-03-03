#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

class MULTIMC_LOGIC_EXPORT FtbPackFetchTask : public QObject {

	Q_OBJECT

public:
	FtbPackFetchTask();
	~FtbPackFetchTask();

	void fetch();

private:
	NetJobPtr jobPtr;
	Net::Download::Ptr downloadPtr;

	QByteArray modpacksXmlFileData;

protected slots:
	void fileDownloadFinished();
	void fileDownloadFailed(QString reason);

signals:
	void finished(FtbModpackList list);
	void failed(QString reason);

};
