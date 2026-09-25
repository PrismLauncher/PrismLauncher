#pragma once
#include "InstanceTask.h"
#include "PackHelpers.h"
#include "minecraft/MinecraftInstance.h"
#include "net/NetJob.h"

#include <memory>
#include <optional>

namespace LegacyFTB {

class PackInstallTask : public InstanceTask {
    Q_OBJECT

   public:
    explicit PackInstallTask(QNetworkAccessManager* network, Modpack pack, QString version);
    ~PackInstallTask() override = default;

    bool canAbort() const override { return true; }
    bool abort() override;

   protected:
    //! Entry point for tasks.
    void executeTask() override;

   private:
    void downloadPack();
    void unzip();
    void install();

   private slots:

    void onUnzipFinished();
    void onUnzipCanceled();

   private: /* data */
    QNetworkAccessManager* m_network;
    bool m_abortable = false;
    QFuture<std::optional<QStringList>> m_extractFuture;
    QFutureWatcher<std::optional<QStringList>> m_extractFutureWatcher;
    NetJob::Ptr m_netJobContainer;
    QString m_archivePath;

    std::unique_ptr<MinecraftInstance> m_instance;

    Modpack m_pack;
    QString m_version;
};

}  // namespace LegacyFTB
