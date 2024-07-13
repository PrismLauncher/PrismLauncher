#include "FMLLibrariesTask.h"

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"

#include "Application.h"
#include "BuildConfig.h"

#include "net/ApiDownload.h"

FMLLibrariesTask::FMLLibrariesTask(MinecraftInstance* inst)
{
    m_inst = inst;
    setCapabilities(Capability::Killable);
}

void FMLLibrariesTask::executeTask()
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
    setProgressTotal(fmlLibsToProcess.size() * 2);
    NetJob::Ptr dljob{ new NetJob("FML libraries", APPLICATION->network()) };
    auto metacache = APPLICATION->metacache();
    Net::Download::Options options = Net::Download::Option::MakeEternal;
    for (auto& lib : fmlLibsToProcess) {
        auto entry = metacache->resolveEntry("fmllibs", lib.filename);
        QString urlString = BuildConfig.FMLLIBS_BASE_URL + lib.filename;
        dljob->addNetAction(Net::ApiDownload::makeCached(QUrl(urlString), entry, options));
    }

    connect(dljob.get(), &TaskV2::finished, this, &FMLLibrariesTask::fmllibsFinished);
    connect(dljob.get(), &TaskV2::processedChanged, this, &FMLLibrariesTask::propateProcessedChanged);
    connect(dljob.get(), &TaskV2::totalChanged, this, &FMLLibrariesTask::propateTotalChanged);
    downloadJob.reset(dljob);
    downloadJob->start();
}

void FMLLibrariesTask::fmllibsFinished(TaskV2*)
{
    if (!downloadJob->wasSuccessful()) {
        QStringList failed = downloadJob->getFailedFiles();
        QString failed_all = failed.join("\n");
        emitFailed(
            tr("Failed to download the following files:\n%1\n\nReason:%2\nPlease try again.").arg(failed_all, downloadJob->failReason()));
        downloadJob.reset();
        return;
    }
    downloadJob.reset();
    if (!fmlLibsToProcess.isEmpty()) {
        setStatus(tr("Copying FML libraries into the instance..."));
        MinecraftInstance* inst = (MinecraftInstance*)m_inst;
        auto metacache = APPLICATION->metacache();
        for (auto& lib : fmlLibsToProcess) {
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
            setProgress(progress() + 1);
        }
    }
    emitSucceeded();
}

bool FMLLibrariesTask::doAbort()
{
    if (downloadJob) {
        return downloadJob->abort();
    }
    qWarning() << "Prematurely aborted FMLLibrariesTask";
    return false;
}
