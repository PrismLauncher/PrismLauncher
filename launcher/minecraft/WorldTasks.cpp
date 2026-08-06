#include "WorldTasks.h"

#include "World.h"
#include "WorldList.h"

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
        const bool ok = world.isValid() && world.install(args.targetDir);

        invokeOnMainThread([self, worlds = args.worlds, ok]() {
            if (!self) {
                return;
            }

            if (!ok) {
                self->emitFailed(self->tr("Failed to import world."));
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