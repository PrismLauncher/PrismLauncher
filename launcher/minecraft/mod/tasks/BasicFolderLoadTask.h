#pragma once

#include <QDir>
#include <QMap>
#include <QObject>

#include <memory>

#include "minecraft/mod/Resource.h"

#include "tasks/Task.h"

/** Very simple task that just loads a folder's contents directly. 
 */
class BasicFolderLoadTask : public Task
{
    Q_OBJECT
public:
    struct Result {
        QMap<QString, Resource::Ptr> resources;
    };
    using ResultPtr = std::shared_ptr<Result>;

    [[nodiscard]] ResultPtr result() const {
        return m_result;
    }

public:
    BasicFolderLoadTask(QDir dir) : Task(nullptr, false), m_dir(dir), m_result(new Result) {}
    void executeTask() override
    {
        m_dir.refresh();
        for (auto entry : m_dir.entryInfoList()) {
            auto resource = new Resource(entry);
            m_result->resources.insert(resource->internal_id(), resource);
        }

        emitSucceeded();
    }

private:
    QDir m_dir;
    ResultPtr m_result;
};
