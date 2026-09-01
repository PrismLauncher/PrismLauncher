#include "RestoreInstanceTask.h"
#include "Application.h"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "modplatform/helpers/HashUtils.h"
#include "modplatform/packwiz/Packwiz.h"
#include "net/ApiRequest.h"
#include "net/ChecksumValidator.h"

namespace {
void addDownloadHashValidator(const Net::NetRequest::Ptr& action, const QString& hashFormat, const QString& hash)
{
    switch (Hashing::algorithmFromString(hashFormat)) {
        case Hashing::Algorithm::Md4:
            action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Md4, hash));
            break;
        case Hashing::Algorithm::Md5:
            action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Md5, hash));
            break;
        case Hashing::Algorithm::Sha1:
            action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha1, hash));
            break;
        case Hashing::Algorithm::Sha256:
            action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha256, hash));
            break;
        case Hashing::Algorithm::Sha512:
            action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha512, hash));
            break;
        default:
            break;
    }
}
}  // namespace

RestoreInstanceTask::RestoreInstanceTask(BaseInstance* instance) : Task(true), m_instance(instance)
{
    setAbortable(true);
}

void RestoreInstanceTask::executeTask()
{
    setStatus(tr("Restoring offloaded instance..."));
    qDebug() << "Starting Restore for offloaded instance.";
    NetJob::Ptr job{ new NetJob(tr("Restoring offloaded files"), APPLICATION->network()) };
    m_netJob.reset(job);

    m_disabledFilenames = m_instance->getOffloadedDisabledFiles();

    // Manifest for all the curseforge files
    Flame::Manifest cfManifest;

    restoreResourceFolder("mods", cfManifest);
    restoreResourceFolder("resourcepacks", cfManifest);
    restoreResourceFolder("shaderpacks", cfManifest);

    // Resolve CurseForge files
    if (!cfManifest.files.isEmpty()) {
        resolveCurseForgeFiles(cfManifest);
    } else {
        // No CurseForge files, just start the download job
        qDebug() << "No CurseForge Files, starting download job.";
        startDownloadJob();
    }
}

void RestoreInstanceTask::restoreResourceFolder(const QString& folderName, Flame::Manifest& manifest)
{
    const QString resourcePath = FS::PathCombine(m_instance->instanceRoot(), "minecraft", folderName);

    QString indexPath;
    if (folderName == "shaderpacks") {
        indexPath = resourcePath;
    } else {
        indexPath = FS::PathCombine(resourcePath, ".index");
    }

    QDir indexDir(indexPath);
    if (!indexDir.exists()) {
        qDebug() << "Attempting to restore resource folder with no .index directory:" << indexPath << "does not exist. Skipping.";
        return;
    }

    const auto indexFiles = indexDir.entryList({ "*.pw.toml" }, QDir::Files);
    if (indexFiles.count() == 0)
    {
        qDebug() << "Metadata directory exists but has no entries. Skipping.";
        return;
    }

    for (const auto& fileName : indexFiles) {
        QString stripped = fileName;
        stripped.remove(".pw.toml");

        auto mod = Packwiz::V1::getIndexForMod(indexPath, stripped);
        if (!mod.isValid()) {
            continue;
        }

        bool isDisabled = false;
        QString targetFile;
        if (m_disabledFilenames.contains(mod.filename))
        {
            // Resource is disabled
            targetFile = FS::PathCombine(resourcePath, mod.filename + ".disabled");
            isDisabled = true;
        }
        else {
            targetFile = FS::PathCombine(resourcePath, mod.filename);
        }

        // Modrinth
        if (mod.mode == "url" && !mod.url.isEmpty()) {
            auto action = Net::ApiRequest::makeFile(mod.url.toString(), targetFile);

            // validate the download if we have the hash
            addDownloadHashValidator(action, mod.hash_format, mod.hash);

            m_netJob->addNetAction(action);

        } else if (mod.mode == "metadata:curseforge") {
            Flame::File cfFile;
            cfFile.projectId = mod.project_id.toInt();
            cfFile.fileId = mod.file_id.toInt();
            cfFile.targetFolder = resourcePath;
            manifest.files.insert(cfFile.fileId, cfFile);

            if (isDisabled) {
                m_disabledCFFileIds.insert(cfFile.fileId);
            }
        } else {
            qDebug() << "Unexpected mod mode when restoring: " << mod.name;
        }
    }
}

void RestoreInstanceTask::resolveCurseForgeFiles(Flame::Manifest& manifest)
{
    if (manifest.files.isEmpty()) {
        qDebug() << "CurseForge manifest is empty. No files to restore.";
        return;
    }

    setStatus(tr("Resolving CurseForge URLs..."));
    qDebug() << "Resolving CurseForge files.";

    auto resolveTask = std::make_shared<Flame::FileResolvingTask>(manifest);
    m_resolveTask = resolveTask;

    connect(resolveTask.get(), &Task::succeeded, this, [this, resolveTask]() {
        auto results = resolveTask->getResults();

        for (const auto& result : results.files) {
            // get file path
            QString fileName;
            if (m_disabledCFFileIds.contains(result.fileId))
            {
                fileName = result.version.fileName + ".disabled";
            }
            else {
                fileName = result.version.fileName;
            }

            const QString targetPath = FS::PathCombine(result.targetFolder, fileName);

            if (result.version.downloadUrl.isEmpty()) {
                qDebug() << "Skipping file with empty download URL (likely a blocked mod with no Modrinth fallback):" << targetPath;
                continue;
            }

            auto action = Net::ApiRequest::makeFile(result.version.downloadUrl, targetPath);

            // validate
            addDownloadHashValidator(action, result.version.hash_type, result.version.hash);

            m_netJob->addNetAction(action);
            qDebug() << "Added action to file:" << targetPath;
        }

        startDownloadJob();
    });

    connect(resolveTask.get(), &Task::failed, this,
            [this](const QString& reason) { emitFailed(tr("Failed to resolve CurseForge files: %1").arg(reason)); });

    resolveTask->start();
}

void RestoreInstanceTask::startDownloadJob()
{
    setStatus(tr("Downloading offloaded files..."));

    // in the chance that there is no action (all blocked mods?)
    if (m_netJob->size() == 0) {
        qDebug() << "NetJob has nothing to do. No files to restore.";
        emitSucceeded();
        return;
    }

    connect(m_netJob.get(), &NetJob::progress, this, &Task::setProgress);
    connect(m_netJob.get(), &NetJob::succeeded, this, &RestoreInstanceTask::emitSucceeded);
    connect(m_netJob.get(), &NetJob::failed, this,
            [this](const QString& reason) { emitFailed(tr("Failed to download restored files: %1").arg(reason)); });

    m_netJob->start();
}

bool RestoreInstanceTask::abort()
{
    if (m_resolveTask) {
        m_resolveTask->abort();
    }
    if (m_netJob) {
        m_netJob->abort();
    }
    return Task::abort();
}
