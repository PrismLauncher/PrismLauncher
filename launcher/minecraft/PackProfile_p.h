#pragma once

#include "Component.h"
#include <map>
#include <QTimer>
#include <QList>
#include <QMap>

class MinecraftInstance;
using ComponentContainer = QList<ComponentPtr>;
using ComponentIndex = QMap<QString, ComponentPtr>;

struct PackProfileData
{
    // the instance this belongs to
    MinecraftInstance *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;

    // the launch profile (volatile, temporary thing created on demand)
    std::shared_ptr<LaunchProfile> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profile;

    // persistent list of components and related machinery
    ComponentContainer components;
    ComponentIndex componentIndex;
    bool dirty = false;
    QTimer hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_saveTimer;
    Task::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask;
    bool loaded = false;
    bool interactionDisabled = true;
};

