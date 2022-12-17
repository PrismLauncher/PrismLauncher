#pragma once

#include <cstddef>
#include <QString>
#include <QList>
#include "net/Mode.h"

class PackProfile;

struct RemoteLoadStatus
{
    enum class Type
    {
        Index,
        List,
        Version
    } type = Type::Version;
    size_t PackProfileIndex = 0;
    bool finished = false;
    bool succeeded = false;
    QString error;
};

struct ComponentUpdateTaskData
{
    PackProfile * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_list = nullptr;
    QList<RemoteLoadStatus> remoteLoadStatusList;
    bool remoteLoadSuccessful = true;
    size_t remoteTasksInProgress = 0;
    ComponentUpdateTask::Mode mode;
    Net::Mode netmode;
};
