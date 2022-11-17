#include "BlockedModsDialog.h"
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QPushButton>
#include "Application.h"
#include "ui_BlockedModsDialog.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

BlockedModsDialog::BlockedModsDialog(QWidget* parent, const QString& title, const QString& text, QList<BlockedMod>& mods)
    : QDialog(parent), ui(new Ui::BlockedModsDialog), m_mods(mods)
{
    m_hashing_task = shared_qobject_ptr<ConcurrentTask>(new ConcurrentTask(this, "MakeHashesTask", 10));
    connect(m_hashing_task.get(), &Task::finished, this, &BlockedModsDialog::hashTaskFinished);

    ui->setupUi(this);

    auto openAllButton = ui->buttonBox->addButton(tr("Open All"), QDialogButtonBox::ActionRole);
    connect(openAllButton, &QPushButton::clicked, this, &BlockedModsDialog::openAll);

    auto downloadFolderButton = ui->buttonBox->addButton(tr("Add Download Folder"), QDialogButtonBox::ActionRole);
    connect(downloadFolderButton, &QPushButton::clicked, this, &BlockedModsDialog::addDownloadFolder);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &BlockedModsDialog::directoryChanged);

    qDebug() << "[Blocked Mods Dialog] Mods List: " << mods;

    setupWatch();
    scanPaths();

    this->setWindowTitle(title);
    ui->labelDescription->setText(text);
    ui->labelExplain->setText(
        QString(tr("Your configured global mods folder and default downloads folder "
                   "are automatically checked for the downloaded mods and they will be copied to the instance if found.<br/>"
                   "Optionally, you may drag and drop the downloaded mods onto this dialog or add a folder to watch "
                   "if you did not download the mods to a default location."))
            .arg(APPLICATION->settings()->get("CentralModsDir").toString(),
                 QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));

    // force all URL handeling as external
    connect(ui->textBrowserWatched, &QTextBrowser::anchorClicked, this, [](const QUrl url) { QDesktopServices::openUrl(url); });

    setAcceptDrops(true);

    update();
}

BlockedModsDialog::~BlockedModsDialog()
{
    delete ui;
}

void BlockedModsDialog::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void BlockedModsDialog::dropEvent(QDropEvent* e)
{
    for (const QUrl& url : e->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        qDebug() << "[Blocked Mods Dialog] Dropped file:" << filePath;
        addHashTask(filePath);

        // watch for changes
        QFileInfo file = QFileInfo(filePath);
        QString path = file.dir().absolutePath();
        qDebug() << "[Blocked Mods Dialog] Adding watch path:" << path;
        m_watcher.addPath(path);
    }
    scanPaths();
    update();
}

void BlockedModsDialog::openAll()
{
    for (auto& mod : m_mods) {
        QDesktopServices::openUrl(mod.websiteUrl);
    }
}

void BlockedModsDialog::addDownloadFolder()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select directory where you downloaded the mods"),
                                          QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), QFileDialog::ShowDirsOnly);
    qDebug() << "[Blocked Mods Dialog] Adding watch path:" << dir;
    m_watcher.addPath(dir);
    scanPath(dir, true);
    update();
}

/// @brief update UI with current status of the blocked mod detection
void BlockedModsDialog::update()
{
    QString text;
    QString span;

    for (auto& mod : m_mods) {
        if (mod.matched) {
            // &#x2714; -> html for HEAVY CHECK MARK : ✔
            span = QString(tr("<span style=\"color:green\"> &#x2714; Found at %1 </span>")).arg(mod.localPath);
        } else {
            // &#x2718; -> html for HEAVY BALLOT X : ✘
            span = QString(tr("<span style=\"color:red\"> &#x2718; Not Found </span>"));
        }
        text += QString(tr("%1: <a href='%2'>%2</a> <p>Hash: %3 %4</p> <br/>")).arg(mod.name, mod.websiteUrl, mod.hash, span);
    }

    ui->textBrowserModsListing->setText(text);

    QString watching;
    for (auto& dir : m_watcher.directories()) {
        watching += QString("<a href=\"%1\">%1</a><br/>").arg(dir);
    }

    ui->textBrowserWatched->setText(watching);

    if (allModsMatched()) {
        ui->labelModsFound->setText("<span style=\"color:green\">✔</span>" + tr("All mods found"));
    } else {
        ui->labelModsFound->setText(tr("Please download the missing mods."));
    }
}

/// @brief Signal fired when a watched direcotry has changed
/// @param path the path to the changed directory
void BlockedModsDialog::directoryChanged(QString path)
{
    qDebug() << "[Blocked Mods Dialog] Directory changed: " << path;
    validateMatchedMods();
    scanPath(path, true);
}

/// @brief add the user downloads folder and the global mods folder to the filesystem watcher
void BlockedModsDialog::setupWatch()
{
    const QString downloadsFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString modsFolder = APPLICATION->settings()->get("CentralModsDir").toString();
    m_watcher.addPath(downloadsFolder);
    m_watcher.addPath(modsFolder);
}

/// @brief scan all watched folder
void BlockedModsDialog::scanPaths()
{
    for (auto& dir : m_watcher.directories()) {
        scanPath(dir, false);
    }
    runHashTask();
}

/// @brief Scan the directory at path, skip paths that do not contain a file name
///        of a blocked mod we are looking for
/// @param path the directory to scan
void BlockedModsDialog::scanPath(QString path, bool start_task)
{
    QDir scan_dir(path);
    QDirIterator scan_it(path, QDir::Filter::Files | QDir::Filter::Hidden, QDirIterator::NoIteratorFlags);
    while (scan_it.hasNext()) {
        QString file = scan_it.next();

        if (!checkValidPath(file)) {
            continue;
        }

        addHashTask(file);
    }

    if (start_task) {
        runHashTask();
    }
}

/// @brief add a hashing task for the file located at path, add the path to the pending set if the hasing task is already running
/// @param path the path to the local file being hashed
void BlockedModsDialog::addHashTask(QString path)
{
    qDebug() << "[Blocked Mods Dialog] adding a Hash task for" << path << "to the pending set.";
    m_pending_hash_paths.insert(path);
}

/// @brief add a hashing task for the file located at path and connect it to check that hash against
///        our blocked mods list
/// @param path the path to the local file being hashed
void BlockedModsDialog::buildHashTask(QString path)
{
    auto hash_task = Hashing::createBlockedModHasher(path, ModPlatform::Provider::FLAME, "sha1");

    qDebug() << "[Blocked Mods Dialog] Creating Hash task for path: " << path;

    connect(hash_task.get(), &Task::succeeded, this, [this, hash_task, path] { checkMatchHash(hash_task->getResult(), path); });
    connect(hash_task.get(), &Task::failed, this, [path] { qDebug() << "Failed to hash path: " << path; });

    m_hashing_task->addTask(hash_task);
}

/// @brief check if the computed hash for the provided path matches a blocked
///        mod we are looking for
/// @param hash the computed hash for the provided path
/// @param path the path to the local file being compared
void BlockedModsDialog::checkMatchHash(QString hash, QString path)
{
    bool match = false;

    qDebug() << "[Blocked Mods Dialog] Checking for match on hash: " << hash << "| From path:" << path;

    for (auto& mod : m_mods) {
        if (mod.matched) {
            continue;
        }
        if (mod.hash.compare(hash, Qt::CaseInsensitive) == 0) {
            mod.matched = true;
            mod.localPath = path;
            match = true;

            qDebug() << "[Blocked Mods Dialog] Hash match found:" << mod.name << hash << "| From path:" << path;

            break;
        }
    }

    if (match) {
        update();
    }
}

/// @brief Check if the name of the file at path matches the name of a blocked mod we are searching for
/// @param path the path to check
/// @return boolean: did the path match the name of a blocked mod?
bool BlockedModsDialog::checkValidPath(QString path)
{
    QFileInfo file = QFileInfo(path);
    QString filename = file.fileName();

    for (auto& mod : m_mods) {
        if (mod.name.compare(filename, Qt::CaseInsensitive) == 0) {
            qDebug() << "[Blocked Mods Dialog] Name match found:" << mod.name << "| From path:" << path;
            return true;
        }
    }

    return false;
}

bool BlockedModsDialog::allModsMatched()
{
    return std::all_of(m_mods.begin(), m_mods.end(), [](auto const& mod) { return mod.matched; });
}

/// @brief ensure matched file paths still exist
void BlockedModsDialog::validateMatchedMods()
{
    bool changed = false;
    for (auto& mod : m_mods) {
        if (mod.matched) {
            QFileInfo file = QFileInfo(mod.localPath);
            if (!file.exists() || !file.isFile()) {
                qDebug() << "[Blocked Mods Dialog] File" << mod.localPath << "for mod" << mod.name
                         << "has vanshed! marking as not matched.";
                mod.localPath = "";
                mod.matched = false;
                changed = true;
            }
        }
    }
    if (changed) {
        update();
    }
}

/// @brief run hash task or mark a pending run if it is already runing
void BlockedModsDialog::runHashTask()
{
    if (!m_hashing_task->isRunning()) {
        m_rehash_pending = false;

        if (!m_pending_hash_paths.isEmpty()) {
            qDebug() << "[Blocked Mods Dialog] there are pending hash tasks, building and running tasks";

            auto path = m_pending_hash_paths.begin();
            while (path != m_pending_hash_paths.end()) {
                buildHashTask(*path);
                path = m_pending_hash_paths.erase(path);
            }

            m_hashing_task->start();
        }
    } else {
        qDebug() << "[Blocked Mods Dialog] queueing another run of the hashing task";
        qDebug() << "[Blocked Mods Dialog] pending hash tasks:" << m_pending_hash_paths;
        m_rehash_pending = true;
    }
}

void BlockedModsDialog::hashTaskFinished()
{
    qDebug() << "[Blocked Mods Dialog] All hash tasks finished";
    if (m_rehash_pending) {
        qDebug() << "[Blocked Mods Dialog] task finished with a rehash pending, rerunning";
        runHashTask();
    }
}

/// qDebug print support for the BlockedMod struct
QDebug operator<<(QDebug debug, const BlockedMod& m)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "{ name: " << m.name << ", websiteUrl: " << m.websiteUrl << ", hash: " << m.hash << ", matched: " << m.matched
                    << ", localPath: " << m.localPath << "}";

    return debug;
}
