#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QThread>

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

    [[nodiscard]] ResultPtr result() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result; }

   public:
    BasicFolderLoadTask(QDir dir) : Task(nullptr, false), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dir(dir), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result(new Result), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into(thread())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_create_func = [](QFileInfo const& entry) -> Resource* {
                return new Resource(entry);
            };
    }
    BasicFolderLoadTask(QDir dir, std::function<Resource*(QFileInfo const&)> create_function)
        : Task(nullptr, false), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dir(dir), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result(new Result), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_create_func(std::move(create_function)), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into(thread())
    {}

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted.store(true);
        return true;
    }

    void executeTask() override
    {
        if (thread() != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into)
            connect(this, &Task::finished, this->thread(), &QThread::quit);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dir.refresh();
        for (auto entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dir.entryInfoList()) {
            auto resource = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_create_func(entry);
            resource->moveToThread(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->resources.insert(resource->internal_id(), resource);
        }

        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted)
            emit finished();
        else
            emitSucceeded();
    }

private:
    QDir hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dir;
    ResultPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result;

    std::atomic<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;

    std::function<Resource*(QFileInfo const&)> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_create_func;

    /** This is the thread in which we should put new mod objects */
    QThread* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into;
};
