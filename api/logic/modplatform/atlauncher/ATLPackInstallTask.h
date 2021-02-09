#pragma once

#include <meta/VersionList.h>
#include "ATLPackManifest.h"

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include "settings/INISettingsObject.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "meta/Version.h"

#include <nonstd/optional>

namespace ATLauncher {

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
Q_OBJECT

public:
    explicit PackInstallTask(QString pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

    void onModsDownloaded();
    void onModsExtracted();

private:
    QString getDirForModType(ModType type, QString raw);
    QString getVersionForLoader(QString uid);
    QString detectLibrary(VersionLibrary library);

    bool createLibrariesComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile);
    bool createPackComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile);

    void installConfigs();
    void extractConfigs();
    void downloadMods();
    bool extractMods(
        const QMap<QString, VersionMod> &toExtract,
        const QMap<QString, VersionMod> &toDecomp,
        const QMap<QString, QString> &toCopy
    );
    void install();

private:
    NetJobPtr jobPtr;
    QByteArray response;

    QString m_pack;
    QString m_version_name;
    PackVersion m_version;

    QMap<QString, VersionMod> modsToExtract;
    QMap<QString, VersionMod> modsToDecomp;
    QMap<QString, QString> modsToCopy;

    QString archivePath;
    QStringList jarmods;
    Meta::VersionPtr minecraftVersion;
    QMap<QString, Meta::VersionPtr> componentsToInstall;

    QFuture<nonstd::optional<QStringList>> m_extractFuture;
    QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;

    QFuture<bool> m_modExtractFuture;
    QFutureWatcher<bool> m_modExtractFutureWatcher;

    QFuture<bool> m_decompFuture;
    QFutureWatcher<bool> m_decompFutureWatcher;

};

}
