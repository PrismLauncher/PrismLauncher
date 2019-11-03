#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

namespace LegacyFTB {

class MULTIMC_LOGIC_EXPORT PackFetchTask : public QObject {

    Q_OBJECT

public:
    PackFetchTask() = default;
    virtual ~PackFetchTask() = default;

    void fetch();
    void fetchPrivate(const QStringList &toFetch);

private:
    NetJobPtr jobPtr;

    QByteArray publicModpacksXmlFileData;
    QByteArray thirdPartyModpacksXmlFileData;

    bool parseAndAddPacks(QByteArray &data, PackType packType, ModpackList &list);
    ModpackList publicPacks;
    ModpackList thirdPartyPacks;

protected slots:
    void fileDownloadFinished();
    void fileDownloadFailed(QString reason);

signals:
    void finished(ModpackList publicPacks, ModpackList thirdPartyPacks);
    void failed(QString reason);

    void privateFileDownloadFinished(Modpack modpack);
    void privateFileDownloadFailed(QString reason, QString packCode);
};

}
