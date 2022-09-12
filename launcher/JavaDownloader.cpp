#include "JavaDownloader.h"
#include <QMessageBox>
#include <QPushButton>
#include <memory>
#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "MMCZip.h"
#include "SysInfo.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "quazip.h"
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

    auto netJob = new NetJob(QString("JRE::QueryVersions"), APPLICATION->network());
    auto response = new QByteArray();
    setStatus(tr("Querying mojang meta"));
    netJob->addNetAction(Net::Download::makeByteArray(
        QUrl("https://piston-meta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"), response));

    QObject::connect(this, &Task::aborted, [isLegacy]{
        QDir(FS::PathCombine("java",(isLegacy ? "java-legacy" : "java-current"))).removeRecursively();
    });

    QObject::connect(netJob, &NetJob::finished, [netJob, response, this] {
        //delete so that it's not called on a deleted job
        QObject::disconnect(this, &Task::aborted, netJob, &NetJob::abort);
        netJob->deleteLater();
        delete response;
    });
    QObject::connect(netJob, &NetJob::progress, this, &JavaDownloader::progress);
    QObject::connect(netJob, &NetJob::failed, this, &JavaDownloader::emitFailed);

    QObject::connect(this, &Task::aborted, netJob, &NetJob::abort);

    QObject::connect(netJob, &NetJob::succeeded, [response, OS, isLegacy, this, netJob] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }
        auto versionArray = Json::ensureArray(Json::ensureObject(doc.object(), OS), isLegacy ? "jre-legacy" : "java-runtime-gamma");
        if (!versionArray.empty()) {
            setStatus(tr("Downloading java from Mojang"));
            auto url = versionArray[0].toObject()["manifest"].toObject()["url"].toString();
            auto download = new NetJob(QString("JRE::DownloadJava"), APPLICATION->network());
            auto files = new QByteArray();

            download->addNetAction(Net::Download::makeByteArray(QUrl(url), files));

            QObject::connect(download, &NetJob::finished, [download, files, this] {
                QObject::disconnect(this, &Task::aborted, download, &NetJob::abort);
                download->deleteLater();
                delete files;
            });
            QObject::connect(download, &NetJob::progress, this, &JavaDownloader::progress);
            QObject::connect(download, &NetJob::failed, this, &JavaDownloader::emitFailed);
            QObject::connect(this, &Task::aborted, download, &NetJob::abort);

            QObject::connect(download, &NetJob::succeeded, [download, files, isLegacy, this] {
                QJsonParseError parse_error{};
                QJsonDocument doc = QJsonDocument::fromJson(*files, &parse_error);
                if (parse_error.error != QJsonParseError::NoError) {
                    qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
                    qWarning() << *files;
                    return;
                }

                // valid json doc, begin making jre spot
                auto output = FS::PathCombine(QString("java"), (isLegacy ? "java-legacy" : "java-current"));
                FS::ensureFolderPathExists(output);
                std::vector<File> toDownload;
                auto list = doc.object()["files"].toObject();
                for (const auto& paths : list.keys()) {
                    auto file = FS::PathCombine(output, paths);

                    auto type = list[paths].toObject()["type"].toString();
                    if (type == "directory") {
                        FS::ensureFolderPathExists(file);
                    } else if (type == "link") {
                        // this is linux only !
                        auto target = FS::PathCombine(file, "../" + list[paths].toObject()["target"].toString());
                        QFile(target).link(file);
                    } else if (type == "file") {
                        // TODO download compressed version if it exists ?
                        auto raw = list[paths].toObject()["downloads"].toObject()["raw"].toObject();
                        auto isExec = list[paths].toObject()["executable"].toBool();
                        auto f = File{ file, raw["url"].toString(), QByteArray::fromHex(raw["sha1"].toString().toLatin1()), isExec };
                        toDownload.push_back(f);
                    }
                }
                auto elementDownload = new NetJob("JRE::FileDownload", APPLICATION->network());
                for (const auto& file : toDownload) {
                    auto dl = Net::Download::makeFile(file.url, file.path);
                    dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.hash));
                    if (file.isExec) {
                        QObject::connect(dl.get(), &Net::Download::succeeded, [file] {
                            QFile(file.path).setPermissions(QFile(file.path).permissions() | QFileDevice::Permissions(0x1111));
                        });
                    }
                    elementDownload->addNetAction(dl);
                }
                QObject::connect(elementDownload, &NetJob::finished, [elementDownload, this] {
                    QObject::disconnect(this, &Task::aborted, elementDownload, &NetJob::abort);
                    elementDownload->deleteLater();
                });
                QObject::connect(elementDownload, &NetJob::progress, this, &JavaDownloader::progress);
                QObject::connect(elementDownload, &NetJob::failed, this, &JavaDownloader::emitFailed);


                QObject::connect(this, &Task::aborted, elementDownload, &NetJob::abort);
                QObject::connect(elementDownload, &NetJob::succeeded, [this] { emitSucceeded(); });
                elementDownload->start();
            });
            download->start();
        } else {
            // mojang does not have a JRE for us, let's get azul zulu
            setStatus(tr("Querying Azul meta"));
            QString javaVersion = isLegacy ? QString("8.0") : QString("17.0");
            QString azulOS;
            QString arch;
            QString bitness;

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
            } else if (OS == "linux"){
                // linux x86 64 (used for debugging, should never reach here)
                azulOS = "linux";
                arch = "x86";
                bitness = "64";
            }
            auto metaResponse = new QByteArray();
            auto downloadJob = new NetJob(QString("JRE::QueryAzulMeta"), APPLICATION->network());
            downloadJob->addNetAction(
                Net::Download::makeByteArray(QString("https://api.azul.com/zulu/download/community/v1.0/bundles/?"
                                                     "java_version=%1"
                                                     "&os=%2"
                                                     "&arch=%3"
                                                     "&hw_bitness=%4"
                                                     "&ext=zip"  // as a zip for all os, even linux NOTE !! Linux ARM is .deb only !!
                                                     "&bundle_type=jre"  // jre only
                                                     "&latest=true"      // only get the one latest entry
                                                     )
                                                 .arg(javaVersion, azulOS, arch, bitness),
                                             metaResponse));
            QObject::connect(downloadJob, &NetJob::finished, [downloadJob, metaResponse, this] {
                QObject::disconnect(this, &Task::aborted, downloadJob, &NetJob::abort);
                downloadJob->deleteLater();
                delete metaResponse;
            });
            QObject::connect(this, &Task::aborted, downloadJob, &NetJob::abort);
            QObject::connect(netJob, &NetJob::failed, this, &JavaDownloader::emitFailed);
            QObject::connect(downloadJob, &NetJob::progress, this, &JavaDownloader::progress);
            QObject::connect(downloadJob, &NetJob::succeeded, [metaResponse, isLegacy, this, downloadJob] {
                QJsonParseError parse_error{};
                QJsonDocument doc = QJsonDocument::fromJson(*metaResponse, &parse_error);
                if (parse_error.error != QJsonParseError::NoError) {
                    qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
                    qWarning() << *metaResponse;
                    return;
                }
                auto array = doc.array();
                if (!array.empty()) {
                    // JRE found ! download the zip
                    setStatus(tr("Downloading java from Azul"));
                    auto downloadURL = QUrl(array[0].toObject()["url"].toString());
                    auto download = new NetJob(QString("JRE::DownloadJava"), APPLICATION->network());
                    auto temp = std::make_unique<QTemporaryFile>(FS::PathCombine(APPLICATION->root(), "temp", "XXXXXX.zip"));
                    FS::ensureFolderPathExists(FS::PathCombine(APPLICATION->root(),"temp"));
                    // Have to open at least once to generate path
                    temp->open();
                    temp->close();
                    download->addNetAction(Net::Download::makeFile(downloadURL, temp->fileName()));
                    QObject::connect(download, &NetJob::finished, [download, this] {
                        QObject::disconnect(this, &Task::aborted, download, &NetJob::abort);
                        download->deleteLater();
                    });
                    QObject::connect(download, &NetJob::progress, this, &JavaDownloader::progress);
                    QObject::connect(download, &NetJob::failed, this, &JavaDownloader::emitFailed);
                    QObject::connect(this, &Task::aborted, download, &NetJob::abort);
                    QObject::connect(download, &NetJob::succeeded, [isLegacy, file = std::move(temp), downloadURL, this] {
                        setStatus(tr("Extracting java"));
                        auto output = FS::PathCombine(FS::PathCombine(QCoreApplication::applicationDirPath(), "java"),
                                                      isLegacy ? "java-legacy" : "java-current");
                        // This should do all of the extracting and creating folders
                        MMCZip::extractDir(file->fileName(), downloadURL.fileName().chopped(4), output);
                        emitSucceeded();
                    });
                    download->start();
                } else {
                    emitFailed(tr("No suitable JRE found"));
                }
            });
            downloadJob->start();
        }
    });

    netJob->start();
}
void JavaDownloader::abortNetJob(NetJob* elementDownload)
{
    if(elementDownload->isRunning()){
        elementDownload->abort();
    }else{
        emit elementDownload->aborted();
    }
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
    QMessageBox box(QMessageBox::Icon::Question, tr("Java version"),
                    tr("Do you want to download Java version 8 or 17?\n Java 8 is recommended for minecraft versions below 1.17\n Java 17 "
                       "is recommended for minecraft versions above or equal to 1.17"),
                    QMessageBox::NoButton, parent);
    auto yes = box.addButton("Java 17", QMessageBox::AcceptRole);
    auto no = box.addButton("Java 8", QMessageBox::AcceptRole);
    auto both = box.addButton(tr("Download both"), QMessageBox::AcceptRole);
    auto cancel = box.addButton(QMessageBox::Cancel);

    if (QFileInfo::exists(FS::PathCombine(QString("java"), "java-legacy"))) {
        no->setEnabled(false);
    }
    if (QFileInfo::exists(FS::PathCombine(QString("java"), "java-current"))) {
        yes->setEnabled(false);
    }
    if (!yes->isEnabled() || !no->isEnabled()) {
        both->setEnabled(false);
    }
    if (!yes->isEnabled() && !no->isEnabled()) {
        QMessageBox::warning(parent, tr("Already installed !"), tr("Both versions of java are already installed !"));
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

    if (dialog.execWithTask(down) && box.clickedButton() == both) {
        auto dwn = new JavaDownloader(false, version);
        ProgressDialog dg(parent);
        dg.setSkipButton(true, tr("Abort"));
        dg.execWithTask(dwn);
    }
}
