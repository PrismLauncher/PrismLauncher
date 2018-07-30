/* Copyright 2013-2018 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QSet>
#include <QList>

#include "BaseInstance.h"

#include "multimc_logic_export.h"

#include "QObjectPtr.h"

class QFileSystemWatcher;
class InstanceTask;
using InstanceId = QString;
using GroupId = QString;
using InstanceLocator = std::pair<InstancePtr, int>;

enum class InstCreateError
{
    NoCreateError = 0,
    NoSuchVersion,
    UnknownCreateError,
    InstExists,
    CantCreateDir
};

enum class GroupsState
{
    NotLoaded,
    Steady,
    Dirty
};


class MULTIMC_LOGIC_EXPORT InstanceList : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit InstanceList(SettingsObjectPtr settings, const QString & instDir, QObject *parent = 0);
    virtual ~InstanceList();

public:
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    enum AdditionalRoles
    {
        GroupRole = Qt::UserRole,
        InstancePointerRole = 0x34B1CB48, ///< Return pointer to real instance
        InstanceIDRole = 0x34B1CB49 ///< Return id if the instance
    };
    /*!
     * \brief Error codes returned by functions in the InstanceList class.
     * NoError Indicates that no error occurred.
     * UnknownError indicates that an unspecified error occurred.
     */
    enum InstListError
    {
        NoError = 0,
        UnknownError
    };

    InstancePtr at(int i) const
    {
        return m_instances.at(i);
    }

    int count() const
    {
        return m_instances.count();
    }

    InstListError loadList();
    void saveNow();


    InstancePtr getInstanceById(QString id) const;
    QModelIndex getInstanceIndexById(const QString &id) const;
    QStringList getGroups();
    GroupId getInstanceGroup(const InstanceId & id) const;
    void setInstanceGroup(const InstanceId & id, const GroupId& name);

    void deleteGroup(const GroupId & name);
    void deleteInstance(const InstanceId & id);

    // Wrap an instance creation task in some more task machinery and make it ready to be used
    Task * wrapInstanceTask(InstanceTask * task);

    /**
     * Create a new empty staging area for instance creation and @return a path/key top commit it later.
     * Used by instance manipulation tasks.
     */
    QString getStagedInstancePath();

    /**
     * Commit the staging area given by @keyPath to the provider - used when creation succeeds.
     * Used by instance manipulation tasks.
     */
    bool commitStagedInstance(const QString & keyPath, const QString& instanceName, const QString & groupName);

    /**
     * Destroy a previously created staging area given by @keyPath - used when creation fails.
     * Used by instance manipulation tasks.
     */
    bool destroyStagingPath(const QString & keyPath);

signals:
    void dataIsInvalid();
    void instancesChanged();
    void groupsChanged(QSet<QString> groups);

public slots:
    void on_InstFolderChanged(const Setting &setting, QVariant value);

private slots:
    void propertiesChanged(BaseInstance *inst);
    void providerUpdated();
    void instanceDirContentsChanged(const QString &path);

private:
    int getInstIndex(BaseInstance *inst) const;
    void suspendWatch();
    void resumeWatch();
    void add(const QList<InstancePtr> &list);
    void loadGroupList();
    void saveGroupList();
    QList<InstanceId> discoverInstances();
    InstancePtr loadInstance(const InstanceId& id);

private:
    int m_watchLevel = 0;
    bool m_dirty = false;
    QList<InstancePtr> m_instances;
    QSet<QString> m_groups;

    SettingsObjectPtr m_globalSettings;
    QString m_instDir;
    QFileSystemWatcher * m_watcher;
    QMap<InstanceId, GroupId> m_groupMap;
    QSet<InstanceId> instanceSet;
    bool m_groupsLoaded = false;
    bool m_instancesProbed = false;
};
