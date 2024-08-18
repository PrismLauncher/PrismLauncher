#include "UpdateGlobalDirectoriesTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/CustomMessageBox.h"

class TryCreateSymlinkTask : public Task {
   public:
    explicit TryCreateSymlinkTask(const QString& source, const QString& destination, MinecraftInstance* instance, const QString& setting)
        : m_source(source), m_destination(destination), m_inst(instance), m_setting(setting)
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
        if (FS::checkFolderPathExists(m_destination)) {
            // If it's already a symlink, it might already be correct.
            if (FS::isSymLink(m_destination)) {
                // If the target of the symlink is already the source, there's nothing to do.
                if (FS::getSymLinkTarget(m_destination) == m_source) {
                    emitSucceeded();
                    return;
                }
            } else if (!FS::checkFolderPathEmpty(m_destination)) {
                fail(tr("Failed to create global folder.\nEnsure that \"%1\" is empty.").arg(m_destination));
                return;
            }

            FS::deletePath(m_destination);
        }

        FS::create_link folderLink(m_source, m_destination);
        folderLink.linkRecursively(false);
        folderLink();  // TODO: Error check

        emitSucceeded();
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
};

UpdateGlobalDirectoriesTask::UpdateGlobalDirectoriesTask(MinecraftInstance* inst, QWidget* parent)
    : Task(parent), m_inst(inst), m_parent(parent)
{}

UpdateGlobalDirectoriesTask::~UpdateGlobalDirectoriesTask() {}

void UpdateGlobalDirectoriesTask::executeTask()
{
    auto tasks = makeShared<ConcurrentTask>(this, "UpdateGlobalDirectoriesTask");

    auto screenshotsTask = makeShared<TryCreateSymlinkTask>(FS::PathCombine(APPLICATION->dataRoot(), "screenshots"),
                                                            m_inst->screenshotsDir(), m_inst, "UseGlobalScreenshotsFolder");
    connect(screenshotsTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(screenshotsTask);

    auto savesTask = makeShared<TryCreateSymlinkTask>(FS::PathCombine(APPLICATION->dataRoot(), "saves"), m_inst->worldDir(), m_inst,
                                                      "UseGlobalSavesFolder");
    connect(savesTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(savesTask);

    auto resoucePacksTask = makeShared<TryCreateSymlinkTask>(FS::PathCombine(APPLICATION->dataRoot(), "resourcepacks"),
                                                             m_inst->resourcePacksDir(), m_inst, "UseGlobalResourcePacksFolder");
    connect(resoucePacksTask.get(), &Task::failed, this, &UpdateGlobalDirectoriesTask::notifyFailed);
    tasks->addTask(resoucePacksTask);

    auto texturePacksTask = makeShared<TryCreateSymlinkTask>(FS::PathCombine(APPLICATION->dataRoot(), "resourcepacks"),
                                                             m_inst->texturePacksDir(), m_inst, "UseGlobalResourcePacksFolder");
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
