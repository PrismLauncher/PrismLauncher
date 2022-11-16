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


private:
    Ui::BlockedModsDialog *ui;
    QList<BlockedMod> &mods;
    QFileSystemWatcher watcher;
    shared_qobject_ptr<ConcurrentTask> hashing_task;

    void openAll();
    void update();
    void directoryChanged(QString path);
    void setupWatch();
    void scanPaths();
    void scanPath(QString path);
    void checkMatchHash(QString hash, QString path);

    bool checkValidPath(QString path);
    bool allModsMatched();
};

QDebug operator<<(QDebug debug, const BlockedMod &m);
