#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QThread>

#include <memory>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/mod/Resource.h"

#include "tasks/Task.h"

/** Very simple task that just loads a folder's contents directly.
 */
class BasicFolderLoadTask : public Task {
    Q_OBJECT
   public:
    struct Result {
        QMap<QString, Resource::Ptr> resources;
    };
    using ResultPtr = std::shared_ptr<Result>;

    [[nodiscard]] ResultPtr result() const { return m_result; }

   public:
    BasicFolderLoadTask(QDir dir) : Task(nullptr, false), m_dir(dir), m_result(new Result), m_thread_to_spawn_into(thread())
    {
        m_create_func = [](QFileInfo const& entry) -> Resource::Ptr { return makeShared<Resource>(entry); };
    }
    BasicFolderLoadTask(QDir dir, std::function<Resource::Ptr(QFileInfo const&)> create_function)
        : Task(nullptr, false)
        , m_dir(dir)
        , m_result(new Result)
        , m_create_func(std::move(create_function))
        , m_thread_to_spawn_into(thread())
    {}

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override
    {
        m_aborted.store(true);
        return true;
    }

    void executeTask() override
    {
        if (thread() != m_thread_to_spawn_into)
            connect(this, &Task::finished, this->thread(), &QThread::quit);

        m_dir.refresh();
        for (auto entry : m_dir.entryInfoList()) {
            auto filePath = entry.absoluteFilePath();
            if (auto app = APPLICATION_DYN; app && app->checkQSavePath(filePath)) {
                continue;
            }
            auto newFilePath = FS::getUniqueResourceName(filePath);
            if (newFilePath != filePath) {
                FS::move(filePath, newFilePath);
                entry = QFileInfo(newFilePath);
            }
            auto resource = m_create_func(entry);
            resource->moveToThread(m_thread_to_spawn_into);
            m_result->resources.insert(resource->internal_id(), resource);
        }

        if (m_aborted)
            emit finished();
        else
            emitSucceeded();
    }

   private:
    QDir m_dir;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;

    std::function<Resource::Ptr(QFileInfo const&)> m_create_func;

    /** This is the thread in which we should put new mod objects */
    QThread* m_thread_to_spawn_into;
};
