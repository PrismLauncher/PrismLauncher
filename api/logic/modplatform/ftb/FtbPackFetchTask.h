#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

class MULTIMC_LOGIC_EXPORT FtbPackFetchTask : public QObject {

    Q_OBJECT

public:
    FtbPackFetchTask() = default;
    virtual ~FtbPackFetchTask() = default;

    void fetch();
    void fetchPrivate(const QStringList &toFetch);

private:
    NetJobPtr jobPtr;

    QByteArray publicModpacksXmlFileData;
    QByteArray thirdPartyModpacksXmlFileData;

    bool parseAndAddPacks(QByteArray &data, FtbPackType packType, FtbModpackList &list);
    FtbModpackList publicPacks;
    FtbModpackList thirdPartyPacks;

protected slots:
    void fileDownloadFinished();
    void fileDownloadFailed(QString reason);

signals:
    void finished(FtbModpackList publicPacks, FtbModpackList thirdPartyPacks);
    void failed(QString reason);

    void privateFileDownloadFinished(FtbModpack modpack);
    void privateFileDownloadFailed(QString reason, QString packCode);
};
