#pragma once

#include <QByteArray>
#include <QObject>
#include <QTemporaryDir>
#include <memory>
#include "PackHelpers.h"
#include "net/NetJob.h"

namespace LegacyFTB {

class PackFetchTask : public QObject {
    Q_OBJECT

   public:
    PackFetchTask(QNetworkAccessManager* network) : QObject(nullptr), m_network(network) {};
    virtual ~PackFetchTask() = default;

    void fetch();
    void fetchPrivate(const QStringList& toFetch);

   private:
    QNetworkAccessManager* m_network;
    NetJob::Ptr jobPtr;

    std::unique_ptr<QByteArray> publicModpacksXmlFileData = std::make_unique<QByteArray>();
    std::unique_ptr<QByteArray> thirdPartyModpacksXmlFileData = std::make_unique<QByteArray>();

    bool parseAndAddPacks(QByteArray& data, PackType packType, ModpackList& list);
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

    void privateFileDownloadFinished(const Modpack& modpack);
    void privateFileDownloadFailed(QString reason, QString packCode);
};

}  // namespace LegacyFTB
