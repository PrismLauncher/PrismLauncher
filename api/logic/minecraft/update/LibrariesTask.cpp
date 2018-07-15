#include "Env.h"
#include "LibrariesTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"

LibrariesTask::LibrariesTask(MinecraftInstance * inst)
{
    m_inst = inst;
}

void LibrariesTask::executeTask()
{
    setStatus(tr("Getting the library files from Mojang..."));
    qDebug() << m_inst->name() << ": downloading libraries";
    MinecraftInstance *inst = (MinecraftInstance *)m_inst;

    // Build a list of URLs that will need to be downloaded.
    auto components = inst->getComponentList();
    auto profile = components->getProfile();

    auto job = new NetJob(tr("Libraries for instance %1").arg(inst->name()));
    downloadJob.reset(job);

    auto metacache = ENV.metacache();
    QList<LibraryPtr> brokenLocalLibs;
    QStringList failedFiles;
    auto createJob = [&](const LibraryPtr & lib)
    {
        if(!lib)
        {
            emitFailed(tr("Null jar is specified in the metadata, aborting."));
            return;
        }
        auto dls = lib->getDownloads(currentSystem, metacache.get(), failedFiles, inst->getLocalLibraryPath());
        for(auto dl : dls)
        {
            downloadJob->addNetAction(dl);
        }
    };
    auto createJobs = [&](const QList<LibraryPtr> & libs)
    {
        for (auto lib : libs)
        {
            createJob(lib);
        }
    };
    createJobs(profile->getLibraries());
    createJobs(profile->getNativeLibraries());
    createJobs(profile->getJarMods());
    createJob(profile->getMainJar());

    // FIXME: this is never filled!!!!
    if (!brokenLocalLibs.empty())
    {
        downloadJob.reset();
        QString failed_all = failedFiles.join("\n");
        emitFailed(tr("Some libraries marked as 'local' are missing their jar "
                    "files:\n%1\n\nYou'll have to correct this problem manually. If this is "
                    "an externally tracked instance, make sure to run it at least once "
                    "outside of MultiMC.").arg(failed_all));
        return;
    }
    connect(downloadJob.get(), &NetJob::succeeded, this, &LibrariesTask::emitSucceeded);
    connect(downloadJob.get(), &NetJob::failed, this, &LibrariesTask::jarlibFailed);
    connect(downloadJob.get(), &NetJob::progress, this, &LibrariesTask::progress);
    downloadJob->start();
}

bool LibrariesTask::canAbort() const
{
    return true;
}

void LibrariesTask::jarlibFailed(QString reason)
{
    emitFailed(tr("Game update failed: it was impossible to fetch the required libraries.\nReason:\n%1").arg(reason));
}

bool LibrariesTask::abort()
{
    if(downloadJob)
    {
        return downloadJob->abort();
    }
    else
    {
        qWarning() << "Prematurely aborted LibrariesTask";
    }
    return true;
}
