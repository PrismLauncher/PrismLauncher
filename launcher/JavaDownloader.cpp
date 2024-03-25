#include "JavaDownloader.h"
#include <QMessageBox>
#include <QPushButton>
#include <memory>
#include <utility>
#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "Json.h"
#include "MMCZip.h"
#include "SysInfo.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "ui/dialogs/ProgressDialog.h"

// Quick & dirty struct to store files
struct File {
    QString path;
    QString url;
    QByteArray hash;
    bool isExec;
};

void JavaDownloader::executeTask()
{
    auto OS = m_OS;
    auto isLegacy = m_isLegacy;

    downloadMojangJavaList(OS, isLegacy);
}
void JavaDownloader::downloadMojangJavaList(const QString& OS, bool isLegacy)
{
    auto netJob = makeShared<NetJob>(QString("JRE::QueryVersions"), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();
    setStatus(tr("Querying mojang meta"));
    netJob->addNetAction(Net::Download::makeByteArray(
        QUrl("https://piston-meta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"), response));

    connect(this, &Task::aborted, [isLegacy] {
        QDir(FS::PathCombine(QCoreApplication::applicationDirPath(), "java", (isLegacy ? "java-legacy" : "java-current")))
            .removeRecursively();
    });

    connect(netJob.get(), &NetJob::finished, [netJob, response, this] {
        // delete so that it's not called on a deleted job
        // FIXME: is this needed? qt should handle this
        disconnect(this, &Task::aborted, netJob.get(), &NetJob::abort);
    });
    connect(netJob.get(), &NetJob::progress, this, &JavaDownloader::progress);
    connect(netJob.get(), &NetJob::failed, this, &JavaDownloader::emitFailed);

    connect(this, &Task::aborted, netJob.get(), &NetJob::abort);

    connect(netJob.get(), &NetJob::succeeded, [response, OS, isLegacy, this, netJob] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }
        auto versionArray = Json::ensureArray(Json::ensureObject(doc.object(), OS), isLegacy ? "jre-legacy" : "java-runtime-gamma");
        if (!versionArray.empty()) {
            parseMojangManifest(isLegacy, versionArray);

        } else {
            // mojang does not have a JRE for us, let's get azul zulu
            downloadAzulMeta(OS, isLegacy, netJob.get());
        }
    });

    netJob->start();
}
void JavaDownloader::parseMojangManifest(bool isLegacy, const QJsonArray& versionArray)
{
    setStatus(tr("Downloading Java from Mojang"));
    auto url = Json::ensureString(Json::ensureObject(Json::ensureObject(versionArray[0]), "manifest"), "url");
    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    auto files = std::make_shared<QByteArray>();

    download->addNetAction(Net::Download::makeByteArray(QUrl(url), files));

    connect(download.get(), &NetJob::finished,
            [download, files, this] { disconnect(this, &Task::aborted, download.get(), &NetJob::abort); });
    connect(download.get(), &NetJob::progress, this, &JavaDownloader::progress);
    connect(download.get(), &NetJob::failed, this, &JavaDownloader::emitFailed);
    connect(this, &Task::aborted, download.get(), &NetJob::abort);

    connect(download.get(), &NetJob::succeeded, [files, isLegacy, this] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*files, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *files;
            return;
        }
        downloadMojangJava(isLegacy, doc);
    });
    download->start();
}
void JavaDownloader::downloadMojangJava(bool isLegacy, const QJsonDocument& doc)
{  // valid json doc, begin making jre spot
    auto output = FS::PathCombine(QCoreApplication::applicationDirPath(), QString("java"), (isLegacy ? "java-legacy" : "java-current"));
    FS::ensureFolderPathExists(output);
    std::vector<File> toDownload;
    auto list = Json::ensureObject(Json::ensureObject(doc.object()), "files");
    for (const auto& paths : list.keys()) {
        auto file = FS::PathCombine(output, paths);

        const QJsonObject& meta = Json::ensureObject(list, paths);
        auto type = Json::ensureString(meta, "type");
        if (type == "directory") {
            FS::ensureFolderPathExists(file);
        } else if (type == "link") {
            // this is linux only !
            auto path = Json::ensureString(meta, "target");
            if (!path.isEmpty()) {
                auto target = FS::PathCombine(file, "../" + path);
                QFile(target).link(file);
            }
        } else if (type == "file") {
            // TODO download compressed version if it exists ?
            auto raw = Json::ensureObject(Json::ensureObject(meta, "downloads"), "raw");
            auto isExec = Json::ensureBoolean(meta, "executable", false);
            auto url = Json::ensureString(raw, "url");
            if (!url.isEmpty() && QUrl(url).isValid()) {
                auto f = File{ file, url, QByteArray::fromHex(Json::ensureString(raw, "sha1").toLatin1()), isExec };
                toDownload.push_back(f);
            }
        }
    }
    auto elementDownload = new NetJob("JRE::FileDownload", APPLICATION->network());
    for (const auto& file : toDownload) {
        auto dl = Net::Download::makeFile(file.url, file.path);
        if (!file.hash.isEmpty()) {
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.hash));
        }
        if (file.isExec) {
            connect(dl.get(), &Net::Download::succeeded,
                    [file] { QFile(file.path).setPermissions(QFile(file.path).permissions() | QFileDevice::Permissions(0x1111)); });
        }
        elementDownload->addNetAction(dl);
    }
    connect(elementDownload, &NetJob::finished, [elementDownload, this] {
        disconnect(this, &Task::aborted, elementDownload, &NetJob::abort);
        elementDownload->deleteLater();
    });
    connect(elementDownload, &NetJob::progress, this, &JavaDownloader::progress);
    connect(elementDownload, &NetJob::failed, this, &JavaDownloader::emitFailed);

    connect(this, &Task::aborted, elementDownload, &NetJob::abort);
    connect(elementDownload, &NetJob::succeeded, [this] { emitSucceeded(); });
    elementDownload->start();
}
void JavaDownloader::downloadAzulMeta(const QString& OS, bool isLegacy, const NetJob* netJob)
{
    setStatus(tr("Querying Azul meta"));
    QString javaVersion = isLegacy ? QString("8.0") : QString("17.0");

    QString azulOS;
    QString arch;
    QString bitness;

    mojangOStoAzul(OS, azulOS, arch, bitness);
    auto metaResponse = std::make_shared<QByteArray>();
    auto downloadJob = makeShared<NetJob>(QString("JRE::QueryAzulMeta"), APPLICATION->network());
    downloadJob->addNetAction(
        Net::Download::makeByteArray(QString("https://api.azul.com/zulu/download/community/v1.0/bundles/?"
                                             "java_version=%1"
                                             "&os=%2"
                                             "&arch=%3"
                                             "&hw_bitness=%4"
                                             "&ext=zip"  // as a zip for all os, even linux NOTE !! Linux ARM is .deb or .tar.gz only !!
                                             "&bundle_type=jre"  // jre only
                                             "&latest=true"      // only get the one latest entry
                                             )
                                         .arg(javaVersion, azulOS, arch, bitness),
                                     metaResponse));
    connect(downloadJob.get(), &NetJob::finished,
            [downloadJob, metaResponse, this] { disconnect(this, &Task::aborted, downloadJob.get(), &NetJob::abort); });
    connect(this, &Task::aborted, downloadJob.get(), &NetJob::abort);
    connect(netJob, &NetJob::failed, this, &JavaDownloader::emitFailed);
    connect(downloadJob.get(), &NetJob::progress, this, &JavaDownloader::progress);
    connect(downloadJob.get(), &NetJob::succeeded, [metaResponse, isLegacy, this] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*metaResponse, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *metaResponse;
            return;
        }
        auto array = Json::ensureArray(doc.array());
        if (!array.empty()) {
            downloadAzulJava(isLegacy, array);
        } else {
            emitFailed(tr("No suitable JRE found"));
        }
    });
    downloadJob->start();
}
void JavaDownloader::mojangOStoAzul(const QString& OS, QString& azulOS, QString& arch, QString& bitness)
{
    if (OS == "mac-os-arm64") {
        // macos arm64
        azulOS = "macos";
        arch = "arm";
        bitness = "64";
    } else if (OS == "linux-arm64") {
        // linux arm64
        azulOS = "linux";
        arch = "arm";
        bitness = "64";
    } else if (OS == "linux-arm") {
        // linux arm (32)
        azulOS = "linux";
        arch = "arm";
        bitness = "32";
    } else if (OS == "linux") {
        // linux x86 64 (used for debugging, should never reach here)
        azulOS = "linux";
        arch = "x86";
        bitness = "64";
    }
}
void JavaDownloader::downloadAzulJava(bool isLegacy, const QJsonArray& array)
{  // JRE found ! download the zip
    setStatus(tr("Downloading Java from Azul"));
    auto downloadURL = QUrl(array[0].toObject()["url"].toString());
    auto download = new NetJob(QString("JRE::DownloadJava"), APPLICATION->network());
    auto path = APPLICATION->instances()->getStagedInstancePath();
    auto temp = FS::PathCombine(path, "azulJRE.zip");

    download->addNetAction(Net::Download::makeFile(downloadURL, temp));
    connect(download, &NetJob::finished, [download, this] {
        disconnect(this, &Task::aborted, download, &NetJob::abort);
        download->deleteLater();
    });
    connect(download, &NetJob::aborted, [path] { APPLICATION->instances()->destroyStagingPath(path); });
    connect(download, &NetJob::progress, this, &JavaDownloader::progress);
    connect(download, &NetJob::failed, this, [this, path](QString reason) {
        APPLICATION->instances()->destroyStagingPath(path);
        emitFailed(std::move(reason));
    });
    connect(this, &Task::aborted, download, &NetJob::abort);
    connect(download, &NetJob::succeeded, [isLegacy, temp, downloadURL, path, this] {
        setStatus(tr("Extracting java"));
        auto output = FS::PathCombine(QCoreApplication::applicationDirPath(), "java", isLegacy ? "java-legacy" : "java-current");
        // This should do all of the extracting and creating folders
        MMCZip::extractDir(temp, downloadURL.fileName().chopped(4), output);
        APPLICATION->instances()->destroyStagingPath(path);
        emitSucceeded();
    });
    download->start();
}
void JavaDownloader::showPrompts(QWidget* parent)
{
    QString sys = SysInfo::currentSystem();
    if (sys == "osx") {
        sys = "mac-os";
    }
    QString arch = SysInfo::useQTForArch();
    QString version;
    if (sys == "windows") {
        if (arch == "x86_64") {
            version = "windows-x64";
        } else if (arch == "i386") {
            version = "windows-x86";
        } else {
            // Unknown, maybe arm, appending arch for downloader
            version = "windows-" + arch;
        }
    } else if (sys == "mac-os") {
        if (arch == "arm64") {
            version = "mac-os-arm64";
        } else {
            version = "mac-os";
        }
    } else if (sys == "linux") {
        if (arch == "x86_64") {
            version = "linux";
        } else {
            // will work for i386, and arm(64)
            version = "linux-" + arch;
        }
    } else {
        // ? ? ? ? ? unknown os, at least it won't have a java version on mojang or azul, display warning
        QMessageBox::warning(parent, tr("Unknown OS"),
                             tr("The OS you are running is not supported by Mojang or Azul. Please install Java manually."));
        return;
    }
    // Selection using QMessageBox for java 8 or 17
    QMessageBox box(
        QMessageBox::Icon::Question, tr("Java version"),
        tr("Do you want to download Java version 8 or 17?\n Java 8 is recommended for older Minecraft versions, below 1.17\n Java 17 "
           "is recommended for newer Minecraft versions, starting from 1.17"),
        QMessageBox::NoButton, parent);
    auto yes = box.addButton("Java 17", QMessageBox::AcceptRole);
    auto no = box.addButton("Java 8", QMessageBox::AcceptRole);
    auto both = box.addButton(tr("Download both"), QMessageBox::AcceptRole);
    auto cancel = box.addButton(QMessageBox::Cancel);

    if (QFileInfo::exists(FS::PathCombine(QCoreApplication::applicationDirPath(), QString("java"), "java-legacy"))) {
        no->setEnabled(false);
    }
    if (QFileInfo::exists(FS::PathCombine(QCoreApplication::applicationDirPath(), QString("java"), "java-current"))) {
        yes->setEnabled(false);
    }
    if (!yes->isEnabled() || !no->isEnabled()) {
        both->setEnabled(false);
    }
    if (!yes->isEnabled() && !no->isEnabled()) {
        QMessageBox::information(parent, tr("Already installed!"), tr("Both versions of Java are already installed!"));
        return;
    }
    box.exec();
    if (box.clickedButton() == nullptr || box.clickedButton() == cancel) {
        return;
    }
    bool isLegacy = box.clickedButton() == no;

    auto down = new JavaDownloader(isLegacy, version);
    ProgressDialog dialog(parent);
    dialog.setSkipButton(true, tr("Abort"));
    bool finished_successfully = dialog.execWithTask(down);
    // Run another download task for the other option as well!
    if (finished_successfully && box.clickedButton() == both) {
        auto dwn = new JavaDownloader(false, version);
        ProgressDialog dg(parent);
        dg.setSkipButton(true, tr("Abort"));
        dg.execWithTask(dwn);
    }
}
