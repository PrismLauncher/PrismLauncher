#pragma once

#include <QByteArray>
#include <QObject>
#include <QTemporaryDir>
#include "PackHelpers.h"
#include "net/NetJob.h"

namespace LegacyFTB {

class PackFetchTask : public QObject {
    Q_OBJECT

   public:
    PackFetchTask(QNetworkAccessManager* network) : QObject(nullptr), m_network(network) {};
    ~PackFetchTask() override = default;

    void fetch();
    void fetchPrivate(const QStringList& toFetch);

   private:
    QNetworkAccessManager* m_network;
    NetJob::Ptr jobPtr;

    bool parseAndAddPacks(QByteArray& data, PackType packType, ModpackList& list);
    ModpackList publicPacks;
    ModpackList thirdPartyPacks;

   protected slots:
    void fileDownloadFinished(QByteArray* publicResponse, QByteArray* thirdPartyResponse);
    void fileDownloadFailed(const QString& reason);
    void fileDownloadAborted();

   signals:
    void finished(const ModpackList& publicPacks, const ModpackList& thirdPartyPacks);
    void failed(const QString& reason);
    void aborted();

    void privateFileDownloadFinished(const Modpack& modpack);
    void privateFileDownloadFailed(const QString& reason, const QString& packCode);
};

}  // namespace LegacyFTB
