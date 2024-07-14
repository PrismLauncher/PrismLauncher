#include "AssetUpdateTask.h"

#include "minecraft/AssetsUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/validators/ChecksumValidator.h"

#include "Application.h"

#include "net/ApiDownload.h"
#include "tasks/Task.h"

AssetUpdateTask::AssetUpdateTask(MinecraftInstance* inst)
{
    m_inst = inst;
    setCapabilities(Capability::Killable);
}

void AssetUpdateTask::executeTask()
{
    setStatus(tr("Updating assets index..."));
    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto assets = profile->getMinecraftAssets();
    QUrl indexUrl = assets->url;
    QString localPath = assets->id + ".json";
    auto job = makeShared<NetJob>(tr("Asset index for %1").arg(m_inst->name()), APPLICATION->network());

    auto metacache = APPLICATION->metacache();
    auto entry = metacache->resolveEntry("asset_indexes", localPath);
    entry->setStale(true);
    auto hexSha1 = assets->sha1.toLatin1();
    qDebug() << "Asset index SHA1:" << hexSha1;
    auto dl = Net::ApiDownload::makeCached(indexUrl, entry);
    auto rawSha1 = QByteArray::fromHex(assets->sha1.toLatin1());
    dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
    job->addNetAction(dl);

    downloadJob.reset(job);

    connect(downloadJob.get(), &TaskV2::finished, this, &AssetUpdateTask::assetIndexFinished);
    connect(downloadJob.get(), &TaskV2::processedChanged, this, &AssetUpdateTask::propateProcessedChanged);
    connect(downloadJob.get(), &TaskV2::totalChanged, this, &AssetUpdateTask::propateTotalChanged);
    connect(downloadJob.get(), &TaskV2::stateChanged, this, &AssetUpdateTask::stateChanged);

    qDebug() << m_inst->name() << ": Starting asset index download";
    downloadJob->start();
}

void AssetUpdateTask::assetIndexFinished(TaskV2* t)
{
    if (!t->wasSuccessful()) {
        qDebug() << m_inst->name() << ": Failed asset index download";
        emitFailed(tr("Failed to download the assets index:\n%1").arg(t->failReason()));
        return;
    }
    AssetsIndex index;
    qDebug() << m_inst->name() << ": Finished asset index download";

    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto assets = profile->getMinecraftAssets();

    QString asset_fname = "assets/indexes/" + assets->id + ".json";
    // FIXME: this looks like a job for a generic validator based on json schema?
    if (!AssetsUtils::loadAssetsIndexJson(assets->id, asset_fname, index)) {
        auto metacache = APPLICATION->metacache();
        auto entry = metacache->resolveEntry("asset_indexes", assets->id + ".json");
        metacache->evictEntry(entry);
        emitFailed(tr("Failed to read the assets index!"));
    }

    auto job = index.getDownloadJob();
    if (job) {
        setStatus(tr("Getting the assets files from Mojang..."));
        downloadJob = job;
        connect(downloadJob.get(), &TaskV2::finished, this, [this](TaskV2* t) {
            if (t->wasSuccessful()) {
                emitSucceeded();
            } else {
                emitFailed(tr("Failed to download assets:\n%1").arg(t->failReason()));
            }
        });
        connect(downloadJob.get(), &TaskV2::processedChanged, this, &AssetUpdateTask::propateProcessedChanged);
        connect(downloadJob.get(), &TaskV2::totalChanged, this, &AssetUpdateTask::propateTotalChanged);
        connect(downloadJob.get(), &TaskV2::stateChanged, this, &AssetUpdateTask::stateChanged);

        downloadJob->start();
        return;
    }
    emitSucceeded();
}

bool AssetUpdateTask::doAbort()
{
    if (downloadJob) {
        return downloadJob->abort();
    }
    qWarning() << "Prematurely aborted AssetUpdateTask";
    return true;
}
