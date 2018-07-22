#pragma once

#include "BaseInstanceProvider.h"
#include <QMap>

class QFileSystemWatcher;
class InstanceTask;

class MULTIMC_LOGIC_EXPORT FolderInstanceProvider : public BaseInstanceProvider
{
    Q_OBJECT
public:
    FolderInstanceProvider(SettingsObjectPtr settings, const QString & instDir);

public:
    /// used by InstanceList to @return a list of plausible IDs to probe for
    QList<InstanceId> discoverInstances() override;

    /// used by InstanceList to (re)load an instance with the given @id.
    InstancePtr loadInstance(const InstanceId& id) override;

    // Wrap an instance creation task in some more task machinery and make it ready to be used
    Task * wrapInstanceTask(InstanceTask * task);

    /**
     * Create a new empty staging area for instance creation and @return a path/key top commit it later.
     * Used by instance manipulation tasks.
     */
    QString getStagedInstancePath() override;
    /**
     * Commit the staging area given by @keyPath to the provider - used when creation succeeds.
     * Used by instance manipulation tasks.
     */
    bool commitStagedInstance(const QString & keyPath, const QString& instanceName, const QString & groupName) override;
    /**
     * Destroy a previously created staging area given by @keyPath - used when creation fails.
     * Used by instance manipulation tasks.
     */
    bool destroyStagingPath(const QString & keyPath) override;

public slots:
    void on_InstFolderChanged(const Setting &setting, QVariant value);

private slots:
    void instanceDirContentsChanged(const QString &path);
    void groupChanged();

private: /* methods */
    void loadGroupList() override;
    void saveGroupList() override;

private: /* data */
    QString m_instDir;
    QFileSystemWatcher * m_watcher;
    QMap<InstanceId, GroupId> groupMap;
    QSet<InstanceId> instanceSet;
    bool m_groupsLoaded = false;
    bool m_instancesProbed = false;
};
