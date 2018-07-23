#pragma once


#include <QObject>
#include <QString>
#include <QMap>
#include "BaseInstance.h"
#include "settings/SettingsObject.h"

#include "multimc_logic_export.h"

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

class MULTIMC_LOGIC_EXPORT FolderInstanceProvider : public QObject
{
    Q_OBJECT
public:
    FolderInstanceProvider(SettingsObjectPtr settings, const QString & instDir);
    virtual ~FolderInstanceProvider() = default;

public:
    /// used by InstanceList to @return a list of plausible IDs to probe for
    QList<InstanceId> discoverInstances();

    /// used by InstanceList to (re)load an instance with the given @id.
    InstancePtr loadInstance(const InstanceId& id);

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
    // Emit this when the list of provided instances changed
    void instancesChanged();
    // Emit when the set of groups your provider supplies changes.
    void groupsChanged(QSet<QString> groups);


public slots:
    void on_InstFolderChanged(const Setting &setting, QVariant value);

private slots:
    void instanceDirContentsChanged(const QString &path);
    void groupChanged();

private: /* methods */
    void loadGroupList();
    void saveGroupList();

private: /* data */
    SettingsObjectPtr m_globalSettings;
    QString m_instDir;
    QFileSystemWatcher * m_watcher;
    QMap<InstanceId, GroupId> groupMap;
    QSet<InstanceId> instanceSet;
    bool m_groupsLoaded = false;
    bool m_instancesProbed = false;
};
