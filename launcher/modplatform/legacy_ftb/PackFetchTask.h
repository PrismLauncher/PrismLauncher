#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

namespace LegacyFTB {

class PackFetchTask : public QObject {

    Q_OBJECT

public:
    PackFetchTask(shared_qobject_ptr<QNetworkAccessManager> network) : QObject(nullptr), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network(network) {};
    virtual ~PackFetchTask() = default;

    void fetch();
    void fetchPrivate(const QStringList &toFetch);

private:
    shared_qobject_ptr<QNetworkAccessManager> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network;
    NetJob::Ptr jobPtr;

    QByteArray publicModpacksXmlFileData;
    QByteArray thirdPartyModpacksXmlFileData;

    bool parseAndAddPacks(QByteArray &data, PackType packType, ModpackList &list);
    ModpackList publicPacks;
    ModpackList thirdPartyPacks;

protected slots:
    void fileDownloadFinished();
    void fileDownloadFailed(QString reason);
    void fileDownloadAborted();

signals:
    void finished(ModpackList publicPacks, ModpackList thirdPartyPacks);
    void failed(QString reason);
    void aborted();

    void privateFileDownloadFinished(Modpack modpack);
    void privateFileDownloadFailed(QString reason, QString packCode);
};

}
