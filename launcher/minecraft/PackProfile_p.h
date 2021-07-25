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
    MinecraftInstance *m_instance;

    // the launch profile (volatile, temporary thing created on demand)
    std::shared_ptr<LaunchProfile> m_profile;

    // version information migrated from instance.cfg file. Single use on migration!
    std::map<QString, QString> m_oldConfigVersions;
    QString getOldConfigVersion(const QString& uid) const
    {
        const auto iter = m_oldConfigVersions.find(uid);
        if(iter != m_oldConfigVersions.cend())
        {
            return (*iter).second;
        }
        return QString();
    }

    // persistent list of components and related machinery
    ComponentContainer components;
    ComponentIndex componentIndex;
    bool dirty = false;
    QTimer m_saveTimer;
    shared_qobject_ptr<Task> m_updateTask;
    bool loaded = false;
    bool interactionDisabled = true;
};

