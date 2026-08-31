#include "OffloadInstanceTask.h"
#include <qdir.h>
#include <QTimer>
#include <memory>

#include "BaseInstance.h"
#include "FileSystem.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "modplatform/packwiz/Packwiz.h"


OffloadInstanceTask::OffloadInstanceTask(BaseInstance* baseInstance)
    : Task(true), m_instance(baseInstance)
{

}

void OffloadInstanceTask::executeTask()
{
    setStatus(tr("Scanning for offloadable files..."));

    processResourceFolder("mods");
    processResourceFolder("resourcepacks");
    processResourceFolder("shaderpacks");

    resolveCurseForgeFiles();
}


void OffloadInstanceTask::processResourceFolder(const QString& folderName)
{
    const QString resourcePath = FS::PathCombine(m_instance->instanceRoot(), "minecraft", folderName);
    const QString indexPath = FS::PathCombine(resourcePath, ".index");

    QDir indexDir(indexPath);
    if (!indexDir.exists())
    {
        return;
    }


    const auto indexFiles = indexDir.entryList({"*.pw.toml"}, QDir::Files);
    for (const auto& fileName : indexFiles)
    {
        QString stripped = fileName;
        stripped.remove(".pw.toml");

        // Packwiz uses 'mod' for every resource
        auto mod = Packwiz::V1::getIndexForMod(indexDir, stripped);
        if (!mod.isValid()){
            continue;
        }

        const QString targetFile = FS::PathCombine(resourcePath, mod.filename);
        QFileInfo targetFileInfo(targetFile);
        if(!targetFileInfo.exists()){
            // already missing or offloaded
            qDebug() << "Attempting to offload a file that doesn't exist. Skipping.";
            continue;
        }

        qint64 size = targetFileInfo.size();
        if (mod.mode == "url" && !mod.url.isEmpty())
        {
            // Modrinth
            if (QFile::remove(targetFile))
            {
                m_filesFreed++;
                m_bytesFreed += size;
                qDebug() << "Offloading file: " << targetFile;
            }
            else {
                qWarning() << "Failed to remove offloaded file:" << targetFile;
            }
        }
        else if (mod.mode == "metadata:curseforge")
        {
            // Curseforge, need to check with API first
            m_curseForgeFiles.insert(mod.file_id.toInt(),
                    CurseForgeOffloadTarget {
                    .projectId = mod.project_id.toInt(),
                    .targetFile = targetFile,
                    .fileSize = size});
        }
        else
        {
            qDebug() << "Unexpected mod mode when offloading: " << mod.name;
        }

    }
}


void OffloadInstanceTask::resolveCurseForgeFiles()
{
    if (m_curseForgeFiles.isEmpty())
    {
        finishOffload();
        return;
    }

    setStatus(tr("Checking CurseForge mods for third-party block..."));
    qDebug() << "Resolving CurseForge Files. Count:" << m_curseForgeFiles.count();

    // Create the manifest
    Flame::Manifest manifest;
    QMapIterator<int, CurseForgeOffloadTarget> i(m_curseForgeFiles);
    while (i.hasNext())
    {
        i.next();
        Flame::File f;
        f.fileId = i.key();
        f.projectId = i.value().projectId;
        f.required = true;
        manifest.files.insert(f.fileId, f);
    }

    auto resolveTask =
        std::make_shared<Flame::FileResolvingTask>(manifest);
    m_flameApiJob = resolveTask;

    connect(resolveTask.get(), &Task::succeeded, this,
            [this, resolveTask]() {
                const Flame::Manifest& results = resolveTask->getResults();

                // check if we have the downloadUrl
                for (const auto& resolvedFile : results.files)
                {
                    auto targetInfo =
                        m_curseForgeFiles.value(resolvedFile.fileId);

                    // mod is blocked and there is no Modrinth alternative
                    if (resolvedFile.version.downloadUrl.isEmpty())
                    {
                        qDebug() << "Skipping blocked mod:" << targetInfo.targetFile;
                        continue;
                    }

                    // URL is valid so it's safe to offload
                    if (QFile::remove(targetInfo.targetFile))
                    {
                        m_filesFreed++;
                        m_bytesFreed += targetInfo.fileSize;
                        qDebug() << "Offloaded CurseForge file:" << targetInfo.targetFile;
                    }
                    else {
                        qDebug() << "Failed to offload CurseForge file:"
                            << targetInfo.targetFile;
                    }
                }

                finishOffload();
            });

    connect(resolveTask.get(), &Task::failed, this,
            [this](const QString& reason) {
                emitFailed(tr("Failed to resolve CurseForge files: %1").arg(reason));
            });

    connect(resolveTask.get(), &Task::progress, this, &Task::setProgress);
    connect(resolveTask.get(), &Task::status, this, &Task::setStatus);

    resolveTask->start();

}


void OffloadInstanceTask::finishOffload()
{
    m_instance->setOffloaded(true);
    qDebug() << "Offload Complete. Freed" << m_filesFreed << "files (" << m_bytesFreed << ") bytes";

    // Delay emitSucceeded() to the next event loop iteration.
    // If the task completes synchronously, the ProgressDialog gets destroyed too quickly,
    // which leaves the main window dimmed until the next input event.
    QTimer::singleShot(0, this, [this]() {
        emitSucceeded();
    });
}
