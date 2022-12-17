#pragma once

#include <QDebug>
#include <QObject>

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModDetails.h"

#include "tasks/Task.h"

class LocalModParseTask : public Task
{
    Q_OBJECT
public:
    struct Result {
        ModDetails details;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result;
    }

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    LocalModParseTask(int token, ResourceType type, const QFileInfo & modFile);
    void executeTask() override;

    [[nodiscard]] int token() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token; }

private:
    void processAsZip();
    void processAsFolder();
    void processAsLitemod();

private:
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;
    ResourceType hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type;
    QFileInfo hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modFile;
    ResultPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result;

    std::atomic<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;
};
