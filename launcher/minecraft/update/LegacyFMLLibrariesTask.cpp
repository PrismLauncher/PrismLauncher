#include "LegacyFMLLibrariesTask.h"

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"

#include "Application.h"
#include "BuildConfig.h"

#include "net/ApiDownload.h"

LegacyFMLLibrariesTask::LegacyFMLLibrariesTask(MinecraftInstance* inst)
{
    m_inst = inst;
}
void LegacyFMLLibrariesTask::executeTask()
{
    // Get the mod list
    MinecraftInstance* inst = (MinecraftInstance*)m_inst;
    auto components = inst->getPackProfile();
    auto profile = components->getProfile();

    if (!profile->hasTrait("legacyFML")) {
        emitSucceeded();
        return;
    }

    QString version = components->getComponentVersion("net.minecraft");
    auto& fmlLibsMapping = g_VersionFilterData.fmlLibsMapping;
    if (!fmlLibsMapping.contains(version)) {
        emitSucceeded();
        return;
    }

    auto& libList = fmlLibsMapping[version];

    // determine if we need some libs for FML or forge
    setStatus(tr("Checking for FML libraries..."));
    if (!components->getComponent("net.minecraftforge")) {
        emitSucceeded();
        return;
    }

    // now check the lib folder inside the instance for files.
    for (auto& lib : libList) {
        QFileInfo libInfo(FS::PathCombine(inst->libDir(), lib.filename));
        if (libInfo.exists())
            continue;
        fmlLibsToProcess.append(lib);
    }

    // if everything is in place, there's nothing to do here...
    if (fmlLibsToProcess.isEmpty()) {
        emitSucceeded();
        return;
    }

    // download missing libs to our place
    setStatus(tr("Downloading FML libraries..."));
    NetJob::Ptr dljob{ new NetJob("FML libraries", APPLICATION->network()) };
    auto metacache = APPLICATION->metacache();
    Net::Download::Options options = Net::Download::Option::MakeEternal;
    const QString base = baseUrl();
    for (auto& lib : fmlLibsToProcess) {
        auto entry = metacache->resolveEntry("fmllibs", lib.filename);
        QString urlString = base + lib.filename;
        dljob->addNetAction(Net::ApiDownload::makeCached(QUrl(urlString), entry, options));
    }

    connect(dljob.get(), &NetJob::succeeded, this, &LegacyFMLLibrariesTask::fmllibsFinished);
    connect(dljob.get(), &NetJob::failed, this, &LegacyFMLLibrariesTask::fmllibsFailed);
    connect(dljob.get(), &NetJob::aborted, this, [this] { emitFailed(tr("Aborted")); });
    connect(dljob.get(), &NetJob::progress, this, &LegacyFMLLibrariesTask::progress);
    connect(dljob.get(), &NetJob::stepProgress, this, &LegacyFMLLibrariesTask::propagateStepProgress);
    downloadJob.reset(dljob);
    downloadJob->start();
}

bool LegacyFMLLibrariesTask::canAbort() const
{
    return true;
}

void LegacyFMLLibrariesTask::fmllibsFinished()
{
    downloadJob.reset();
    if (!fmlLibsToProcess.isEmpty()) {
        setStatus(tr("Copying FML libraries into the instance..."));
        MinecraftInstance* inst = (MinecraftInstance*)m_inst;
        auto metacache = APPLICATION->metacache();
        int index = 0;
        for (auto& lib : fmlLibsToProcess) {
            progress(index, fmlLibsToProcess.size());
            auto entry = metacache->resolveEntry("fmllibs", lib.filename);
            auto path = FS::PathCombine(inst->libDir(), lib.filename);
            if (!FS::ensureFilePathExists(path)) {
                emitFailed(tr("Failed creating FML library folder inside the instance."));
                return;
            }
            if (!QFile::copy(entry->getFullPath(), FS::PathCombine(inst->libDir(), lib.filename))) {
                emitFailed(tr("Failed copying Forge/FML library: %1.").arg(lib.filename));
                return;
            }
            index++;
        }
        progress(index, fmlLibsToProcess.size());
    }
    emitSucceeded();
}
void LegacyFMLLibrariesTask::fmllibsFailed(QString reason)
{
    QStringList failed = downloadJob->getFailedFiles();
    QString failed_all = failed.join("\n");
    emitFailed(tr("Failed to download the following files:\n%1\n\nReason:%2\nPlease try again.").arg(failed_all, reason));
}

bool LegacyFMLLibrariesTask::abort()
{
    if (downloadJob) {
        return downloadJob->abort();
    } else {
        qWarning() << "Prematurely aborted LegacyFMLLibrariesTask";
    }
    return true;
}

QString LegacyFMLLibrariesTask::baseUrl()
{
    if (const QString urlOverride = APPLICATION->settings()->get("LegacyFMLLibsURLOverride").toString(); !urlOverride.isEmpty()) {
        return urlOverride;
    }

    return BuildConfig.LEGACY_FMLLIBS_BASE_URL;
}
