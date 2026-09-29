#include "WorldTasks.h"

#include "FileSystem.h"
#include "World.h"
#include "WorldList.h"
#include "archive/ExtractZipTask.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThreadPool>

#include <utility>

namespace {

template <typename Func>
void invokeOnMainThread(Func&& func)
{
    auto app = QCoreApplication::instance();
    if (!app) {
        return;
    }

    QMetaObject::invokeMethod(app, std::forward<Func>(func), Qt::QueuedConnection);
}

}  // namespace

InstallWorldTask::InstallWorldTask(Args args) : m_args(std::move(args)) {}

void InstallWorldTask::executeTask()
{
    setStatus(tr("Importing world..."));
    setDetails(m_args.sourceFile.fileName());
    setProgress(0, 0);

    QPointer<InstallWorldTask> self(this);
    auto args = m_args;

    QThreadPool::globalInstance()->start([self, args]() mutable {
        World world(args.sourceFile);
        world.loadMetadata();

        invokeOnMainThread([self, args, world]() {
            if (!self) {
                return;
            }

            auto finalPath = FS::PathCombine(args.targetDir, FS::DirNameFromString(world.name(), { args.targetDir }));
            if (!world.isValid() || !FS::ensureFolderPathExists(finalPath)) {
                self->emitFailed(self->tr("Failed to import world."));
                return;
            }

            self->m_extractTask =
                makeShared<MMCZip::ExtractZipTask>(args.sourceFile.absoluteFilePath(), QDir(finalPath), world.containerOffsetPath());
            connect(self->m_extractTask.get(), &Task::progress, self, &InstallWorldTask::setProgress);
            connect(self->m_extractTask.get(), &Task::failed, self, &InstallWorldTask::emitFailed);
            connect(self->m_extractTask.get(), &Task::succeeded, self, [self, worlds = args.worlds]() {
                if (worlds) {
                    worlds->update();
                }
                self->emitSucceeded();
            });
            self->m_extractTask->start();
        });
    });
}

CopyWorldTask::CopyWorldTask(Args args) : m_args(std::move(args)) {}

void CopyWorldTask::executeTask()
{
    setStatus(tr("Copying world..."));
    setDetails(m_args.targetName);
    setProgress(0, 0);

    QPointer<CopyWorldTask> self(this);
    auto args = m_args;

    QThreadPool::globalInstance()->start([self, args]() mutable {
        World world(args.sourceFile);
        const bool ok = world.isValid() && world.install(args.targetDir, args.targetName);

        invokeOnMainThread([self, worlds = args.worlds, ok]() {
            if (!self) {
                return;
            }

            if (!ok) {
                self->emitFailed(self->tr("Failed to copy world."));
                return;
            }

            if (worlds) {
                worlds->update();
            }

            self->setProgress(1, 1);
            self->emitSucceeded();
        });
    });
}

DeleteWorldTask::DeleteWorldTask(Args args) : m_args(std::move(args)) {}

void DeleteWorldTask::executeTask()
{
    setStatus(tr("Deleting world..."));
    setDetails(m_args.displayName);
    setProgress(0, 0);

    QPointer<DeleteWorldTask> self(this);
    auto args = m_args;

    QThreadPool::globalInstance()->start([self, args]() mutable {
        World world(args.sourceFile);
        const bool ok = world.destroy();

        invokeOnMainThread([self, worlds = args.worlds, sourceFile = args.sourceFile, ok]() {
            if (!self) {
                return;
            }

            if (!ok) {
                self->emitFailed(self->tr("Failed to delete world."));
                return;
            }

            if (worlds) {
                worlds->removeWorldFromModel(sourceFile);
            }

            self->setProgress(1, 1);
            self->emitSucceeded();
        });
    });
}