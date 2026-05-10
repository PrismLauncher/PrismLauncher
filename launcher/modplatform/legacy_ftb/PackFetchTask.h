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
    explicit PackFetchTask(QNetworkAccessManager* network) : QObject(nullptr), m_network(network) {};
    ~PackFetchTask() override = default;

    void fetch();
    void fetchPrivate(const QStringList& toFetch);

   private:
    QNetworkAccessManager* m_network;
    NetJob::Ptr m_jobPtr;

    static bool parseAndAddPacks(QByteArray& data, PackType packType, ModpackList& list);
    ModpackList m_publicPacks;
    ModpackList m_thirdPartyPacks;

   protected slots:
    void fileDownloadFinished(QByteArray* publicPtr, QByteArray* thirdPartyPtr);
    void fileDownloadFailed(const QString& reason);
    void fileDownloadAborted();

   signals:
    void finished(ModpackList publicPacks, ModpackList thirdPartyPacks);
    void failed(const QString& reason);
    void aborted();

    void privateFileDownloadFinished(const Modpack& modpack);
    void privateFileDownloadFailed(const QString& reason, const QString& packCode);
};

}  // namespace LegacyFTB
