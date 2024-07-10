#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QThread>

#include <memory>

#include "FileSystem.h"
#include "minecraft/mod/Resource.h"

#include "tasks/Task.h"

/** Very simple task that just loads a folder's contents directly.
 */
class BasicFolderLoadTask : public TaskV2 {
    Q_OBJECT
   public:
    struct Result {
        QMap<QString, Resource::Ptr> resources;
    };
    using ResultPtr = std::shared_ptr<Result>;

    [[nodiscard]] ResultPtr result() const { return m_result; }

   public:
    BasicFolderLoadTask(
        QDir dir,
        std::function<Resource::Ptr(QFileInfo const&)> create_function = [](QFileInfo const& entry) -> Resource::Ptr {
            return makeShared<Resource>(entry);
        })
        : TaskV2(nullptr, QtWarningMsg)
        , m_dir(dir)
        , m_result(new Result)
        , m_create_func(std::move(create_function))
        , m_thread_to_spawn_into(thread())
    {}

    void executeTask() override
    {
        if (thread() != m_thread_to_spawn_into)
            connect(this, &TaskV2::finished, this->thread(), &QThread::quit);

        m_dir.refresh();
        for (auto entry : m_dir.entryInfoList()) {
            auto filePath = entry.absoluteFilePath();
            auto newFilePath = FS::getUniqueResourceName(filePath);
            if (newFilePath != filePath) {
                FS::move(filePath, newFilePath);
                entry = QFileInfo(newFilePath);
            }
            auto resource = m_create_func(entry);
            resource->moveToThread(m_thread_to_spawn_into);
            m_result->resources.insert(resource->internal_id(), resource);
        }

        emitSucceeded();
    }

   private:
    QDir m_dir;
    ResultPtr m_result;

    std::function<Resource::Ptr(QFileInfo const&)> m_create_func;

    /** This is the thread in which we should put new mod objects */
    QThread* m_thread_to_spawn_into;
};
