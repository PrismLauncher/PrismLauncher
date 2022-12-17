#pragma once

#include "tasks/Task.h"

class MinecraftInstance;
class FoldersTask : public Task
{
    Q_OBJECT
public:
    FoldersTask(MinecraftInstance * inst);
    virtual ~FoldersTask() {};

    void executeTask() override;
private:
    MinecraftInstance *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst;
};

