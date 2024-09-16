#pragma once

#include <QList>
#include <QString>
#include <cstddef>
#include "net/Mode.h"
#include "tasks/Task.h"

#include "minecraft/ComponentUpdateTask.h"

class PackProfile;

struct RemoteLoadStatus {
    enum class Type { Index, List, Version } type = Type::Version;
    size_t PackProfileIndex = 0;
    bool finished = false;
    bool succeeded = false;
    QString error;
    Task::Ptr task;
};

struct ComponentUpdateTaskData {
    PackProfile* m_profile = nullptr;
    QList<RemoteLoadStatus> remoteLoadStatusList;
    bool remoteLoadSuccessful = true;
    size_t remoteTasksInProgress = 0;
    ComponentUpdateTask::Mode mode;
    Net::Mode netmode;
};
