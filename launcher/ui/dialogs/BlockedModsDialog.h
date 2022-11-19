#pragma once

#include <QDialog>
#include <QString>
#include <QList>

#include <QFileSystemWatcher>

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

struct BlockedMod {
    QString name;
    QString websiteUrl;
    QString hash;
    bool matched;
    QString localPath;

};

QT_BEGIN_NAMESPACE
namespace Ui { class BlockedModsDialog; }
QT_END_NAMESPACE

class BlockedModsDialog : public QDialog {
Q_OBJECT

public:
    BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, QList<BlockedMod> &mods);

    ~BlockedModsDialog() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    Ui::BlockedModsDialog *ui;
    QList<BlockedMod> &m_mods;
    QFileSystemWatcher m_watcher;
    shared_qobject_ptr<ConcurrentTask> m_hashing_task;
    QSet<QString> m_pending_hash_paths;
    bool m_rehash_pending;

    void openAll();
    void addDownloadFolder();
    void update();
    void directoryChanged(QString path);
    void setupWatch();
    void scanPaths();
    void scanPath(QString path, bool start_task);
    void addHashTask(QString path);
    void buildHashTask(QString path);
    void checkMatchHash(QString hash, QString path);
    void validateMatchedMods();
    void runHashTask();
    void hashTaskFinished();

    bool checkValidPath(QString path);
    bool allModsMatched();
};

QDebug operator<<(QDebug debug, const BlockedMod &m);
