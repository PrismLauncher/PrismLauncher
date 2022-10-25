#include <qfileinfo.h>
#include <qnamespace.h>
#include "Application.h"
#include "BlockedModsDialog.h"
#include "ui_BlockedModsDialog.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDesktopServices>

#include <QDebug>
#include <QStandardPaths>




BlockedModsDialog::BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, QList<BlockedMod> &mods) :
        QDialog(parent), ui(new Ui::BlockedModsDialog), mods(mods) {
    ui->setupUi(this);

    auto openAllButton = ui->buttonBox->addButton(tr("Open All"), QDialogButtonBox::ActionRole);
    connect(openAllButton, &QPushButton::clicked, this, &BlockedModsDialog::openAll);

    connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &BlockedModsDialog::directoryChanged);

    hashing_task = shared_qobject_ptr<ConcurrentTask>(new ConcurrentTask(this, "MakeHashesTask", 10));
    
    qDebug() << "Mods List: " << mods;

    setupWatch();
    scanPaths(true);

    this->setWindowTitle(title);
    ui->label->setText(text);
    ui->labelModsFound->setText("Please download the missing mods.");
    update();
}

BlockedModsDialog::~BlockedModsDialog() {
    delete ui;
}

void BlockedModsDialog::openAll() {
    for(auto &mod : mods) {
        QDesktopServices::openUrl(mod.websiteUrl);
    }
}

void BlockedModsDialog::update() {
    QString text;
    QString span;

    for (auto &mod : mods) {
        if (mod.matched) {
            // &#x2714; -> html for HEAVY CHECK MARK : ✔
            span = QString("<span style=\"color:green\"> &#x2714; Found at %1 </span>").arg(mod.localPath);
        } else {
            // &#x2718; -> html for HEAVY BALLOT X : ✘
            span = QString("<span style=\"color:red\"> &#x2718; Not Found </span>");
        }
        text += QString("%1: <a href='%2'>%2</a> <p>Hash: %3 %4</p> <br/>").arg(mod.name, mod.websiteUrl, mod.hash, span);
    }

    ui->textBrowser->setText(text);

    if (allModsMatched()) {
        ui->labelModsFound->setText("All mods found ✔");
    } else {
        ui->labelModsFound->setText("Please download the missing mods.");
    }
}

void BlockedModsDialog::directoryChanged(QString path) {
    qDebug() << "Directory changed: " << path;
    scanPath(path, false);
}


void BlockedModsDialog::setupWatch() {
    const QString downloadsFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString modsFolder = APPLICATION->settings()->get("CentralModsDir").toString();
    watcher.addPath(downloadsFolder);
    watcher.addPath(modsFolder);
}

void BlockedModsDialog::scanPaths(bool init) {
    for (auto &dir : watcher.directories()) {
        scanPath(dir, init);
    }
}

void BlockedModsDialog::scanPath(QString path, bool init) {

    QDir scan_dir(path);
    QDirIterator scan_it(path, QDir::Filter::Files | QDir::Filter::Hidden, QDirIterator::NoIteratorFlags);
    while (scan_it.hasNext()) {
        QString file = scan_it.next();

        if (checked_paths.contains(file)){
            continue;
        }

        if (!checkValidPath(file)) {
            continue;
        }

        auto hash_task = Hashing::createBlockedModHasher(file, ModPlatform::Provider::FLAME, "sha1");

        qDebug() << "Creating Hash task for path: " << file;

        connect(hash_task.get(), &Task::succeeded, [this, hash_task, file] { 
            checkMatchHash(hash_task->getResult(), file);
        });
        connect(hash_task.get(), &Task::failed, [this, hash_task, file] { 
            qDebug() << "Failed to hash path: " << file;
        });

        if (init) {
            checked_paths.insert(file);
        }
        
        hashing_task->addTask(hash_task);
    }

    hashing_task->start();

}

void BlockedModsDialog::checkMatchHash(QString hash, QString path) {
    bool match = false;

    qDebug() << "Checking for match on hash: " << hash << " | From path:" << path;

    for (auto &mod : mods) {
        if (mod.matched) {
            continue;
        }
        if (mod.hash.compare(hash, Qt::CaseInsensitive) == 0) {
            mod.matched = true;
            mod.localPath = path;
            match = true;

            qDebug() << "Hash match found: " << mod.name << " " << hash << " | From path:" << path;

            break;
        }
    }

    if (match) {
        update();
    }
}

bool BlockedModsDialog::checkValidPath(QString path) {

    QFileInfo file = QFileInfo(path);
    QString filename = file.fileName();

    for (auto &mod : mods) {
        if (mod.name.compare(filename, Qt::CaseInsensitive) == 0) {
            qDebug() << "Name match found: " << mod.name << " | From path:" << path;
            return true;
        }
    }

    return false;
}

bool BlockedModsDialog::allModsMatched() {
    for (auto &mod : mods) {
        if (!mod.matched)
            return false;
    }
    return true;
}


QDebug operator<<(QDebug debug, const BlockedMod &m) {
    QDebugStateSaver saver(debug);

    debug.nospace() << "{ name: " << m.name << ", websiteUrl: " << m.websiteUrl
        << ", hash: " << m.hash << ", matched: " << m.matched
        << ", localPath: " << m.localPath <<"}";

    return debug;
}