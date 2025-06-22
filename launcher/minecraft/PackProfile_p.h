#pragma once

#include <QList>
#include <QMap>
#include <QTimer>
#include "Component.h"
#include "tasks/Task.h"

class MinecraftInstance;
using ComponentContainer = QList<ComponentPtr>;
using ComponentIndex = QMap<QString, ComponentPtr>;

struct PackProfileData {
    // the instance this belongs to
    MinecraftInstance* m_instance;

    // the launch profile (volatile, temporary thing created on demand)
    std::shared_ptr<LaunchProfile> m_profile;

    // persistent list of components and related machinery
    ComponentContainer components;
    ComponentIndex componentIndex;
    bool dirty = false;
    QTimer m_saveTimer;
    Task::Ptr m_updateTask;
    bool loaded = false;
    bool interactionDisabled = true;
};
