#include "UpdateGlobalDirectoriesTask.h"

#include <QDirIterator>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/FileConflictDialog.h"

/**
 * @brief Move a file or folder, and ask the user what to do in case of a conflict.
 * @param source      What to move.
 * @param destination Where to move it to.
 * @param recursive   If true, all direct children will be moved 1 by 1.
 *                    If false, the source will be directly moved to the destination.
 * @param parent      The parent of the dialog.
 * @return True if everything could be moved.
 */
bool interactiveMove(const QString& source, const QString& destination, bool recursive = false, QWidget* parent = nullptr)
{
    const QFileInfo sourceInfo(source);

    // Make sure the source exists.
    if (!sourceInfo.exists())
        return false;

    // Recursive doesn't make sense if the source isn't a directory.
    if (recursive && sourceInfo.isDir()) {
        QDirIterator sourceIt(source, QDir::Filter::Files | QDir::Filter::Dirs | QDir::Filter::Hidden | QDir::Filter::NoDotAndDotDot);

        while (sourceIt.hasNext()) {
            if (!interactiveMove(sourceIt.next(), FS::PathCombine(destination, sourceIt.fileName()), false))
                return false;
        }

        return true;
    }

    if (QFile(destination).exists()) {
        FileConflictDialog dialog(source, destination, true, parent);
        FileConflictDialog::Result result = dialog.execWithResult();

        if (result == FileConflictDialog::Cancel)
            return false;
        else if (result == FileConflictDialog::ChooseDestination)
            return FS::deletePath(source);
        else if (result == FileConflictDialog::ChooseSource)
            FS::deletePath(destination);
    }

    return FS::move(source, destination);
}

class TryCreateSymlinkTask : public Task {
   public:
    explicit TryCreateSymlinkTask(const QString& source,
                                  const QString& destination,
                                  MinecraftInstance* instance,
                                  const QString& setting,
                                  QWidget* parent)
        : m_source(source), m_destination(destination), m_inst(instance), m_setting(setting), m_parent(parent)
    {
        setObjectName("TryCreateSymlinkTask");
    }
    virtual ~TryCreateSymlinkTask() {}

   protected:
    void executeTask()
    {
        bool create = m_inst->settings()->get(m_setting).toBool();

        // Check if we have to delete an existing symlink
        if (!create) {
            // Safety check
            if (FS::isSymLink(m_destination)) {
                FS::deletePath(m_destination);
            }

            emitSucceeded();
            return;
        }

        // Make sure that symbolic links are supported.
        if (!FS::canLink(m_source, m_destination)) {
            fail(tr("Failed to create global folder.\nSymbolic links are not supported on the filesystem"));
            return;
        }

        // Check if the destination already exists.
        // If it's already a symlink, it might already be correct.
        if (FS::isSymLink(m_destination)) {
            // If the target of the symlink is already the source, there's nothing to do.
            if (FS::getSymLinkTarget(m_destination) == m_source) {
                emitSucceeded();
                return;
            }

            FS::deletePath(m_destination);
        } else if (FS::checkFolderPathExists(m_destination)) {
            if (!FS::checkFolderPathEmpty(m_destination)) {
                if (!interactiveMove(m_destination, m_source, true, m_parent)) {
                    fail(tr("Failed to create global folder.\nEnsure that \"%1\" is empty.").arg(m_destination));
                    return;
                }
            }

            FS::deletePath(m_destination);
        }

        // Make sure the source folder exists
        if (!FS::ensureFolderPathExists(m_source)) {
            fail(tr("Failed to create global folder.\nEnsure that \"%1\" exists.").arg(m_source));
            return;
        }

        FS::create_link folderLink(m_source, m_destination);
        folderLink.linkRecursively(false);

        if (folderLink()) {
            emitSucceeded();
        } else {
            fail(tr("Failed to create global folder. Error %1: %2")
                     .arg(folderLink.getOSError().value())
                     .arg(folderLink.getOSError().message().c_str()));
        }

        return;
    }

    void fail(const QString& reason)
    {
        m_inst->settings()->set(m_setting, false);
        emitFailed(reason);
    }

   private:
    QString m_source;
    QString m_destination;
    MinecraftInstance* m_inst;
    QString m_setting;
    QWidget* m_parent;
};

UpdateGlobalDirectoriesTask::UpdateGlobalDirectoriesTask(MinecraftInstance* inst, QWidget* parent)
    : Task(parent), m_inst(inst), m_parent(parent)
{}

UpdateGlobalDirectoriesTask::~UpdateGlobalDirectoriesTask() {}

void UpdateGlobalDirectoriesTask::executeTask()
{
    auto tasks = makeShared<ConcurrentTask>(this, "UpdateGlobalDirectoriesTask");

    auto screenshotsTask = makeShared<TryCreateSymlinkTask>(m_inst->settings()->get("GlobalScreenshotsPath").toString(),
                                                            m_inst->screenshotsDir(), m_inst, "UseGlobalScreenshotsFolder", m_parent);
    connect(screenshotsTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(screenshotsTask);

    auto savesTask = makeShared<TryCreateSymlinkTask>(m_inst->settings()->get("GlobalSavesPath").toString(), m_inst->worldDir(), m_inst,
                                                      "UseGlobalSavesFolder", m_parent);
    connect(savesTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(savesTask);

    auto resoucePacksTask = makeShared<TryCreateSymlinkTask>(m_inst->settings()->get("GlobalResourcePacksPath").toString(),
                                                             m_inst->resourcePacksDir(), m_inst, "UseGlobalResourcePacksFolder", m_parent);
    connect(resoucePacksTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(resoucePacksTask);

    auto texturePacksTask = makeShared<TryCreateSymlinkTask>(m_inst->settings()->get("GlobalResourcePacksPath").toString(),
                                                             m_inst->texturePacksDir(), m_inst, "UseGlobalResourcePacksFolder", m_parent);
    connect(texturePacksTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(texturePacksTask);

    m_tasks = tasks;

    connect(m_tasks.get(), &Task::succeeded, this, &UpdateGlobalDirectoriesTask::emitSucceeded);

    m_tasks->start();
}

void UpdateGlobalDirectoriesTask::notifyFailed(QString reason)
{
    CustomMessageBox::selectable(m_parent, tr("Failed"), reason, QMessageBox::Warning, QMessageBox::Ok)->exec();
    emit failed(reason);
}
