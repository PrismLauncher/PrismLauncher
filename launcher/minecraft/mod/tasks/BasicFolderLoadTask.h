#pragma once

#include <QDir>
#include <QMap>
#include <QObject>

#include <memory>

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
    BasicFolderLoadTask(QDir dir) : Task(nullptr, false), m_dir(dir), m_result(new Result)
    {
        m_create_func = [](QFileInfo const& entry) -> Resource* {
                return new Resource(entry);
            };
    }
    BasicFolderLoadTask(QDir dir, std::function<Resource*(QFileInfo const&)> create_function)
        : Task(nullptr, false), m_dir(dir), m_result(new Result), m_create_func(std::move(create_function))
    {}

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override
    {
        m_aborted = true;
        return true;
    }

    void executeTask() override
    {
        m_dir.refresh();
        for (auto entry : m_dir.entryInfoList()) {
            auto resource = m_create_func(entry);
            m_result->resources.insert(resource->internal_id(), resource);
        }

        if (m_aborted)
            emitAborted();
        else
            emitSucceeded();
    }

private:
    QDir m_dir;
    ResultPtr m_result;

    bool m_aborted = false;

    std::function<Resource*(QFileInfo const&)> m_create_func;
};
