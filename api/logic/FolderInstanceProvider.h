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


    /*
    // create instance in this provider
    Task * creationTask(BaseVersionPtr version, const QString &instName, const QString &instGroup, const QString &instIcon);

    // copy instance to this provider
    Task * copyTask(const InstancePtr &oldInstance, const QString& instName, const QString& instGroup, const QString& instIcon, bool copySaves);

    // import zipped instance into this provider
    Task * zipImportTask(const QUrl sourceUrl, const QString &instName, const QString &instGroup, const QString &instIcon);

    //create FtbInstance
    Task * ftbCreationTask(FtbPackDownloader *downloader, const QString &instName, const QString &instGroup, const QString &instIcon);

    // migrate an instance to the current format
    Task * legacyUpgradeTask(const InstancePtr& oldInstance);
*/

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
    QMap<QString, QString> groupMap;
    bool m_groupsLoaded = false;
};
