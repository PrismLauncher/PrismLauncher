#include "JavaDownloader.h"
#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "MMCZip.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "quazip.h"

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
    QObject::connect(netJob, &NetJob::finished, [netJob, response] {
        netJob->deleteLater();
        delete response;
    });
    QObject::connect(netJob, &NetJob::progress, this, &JavaDownloader::progress);
    QObject::connect(netJob, &NetJob::succeeded, [response, OS, isLegacy, this] {
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

            QObject::connect(download, &NetJob::finished, [download, files] {
                download->deleteLater();
                delete files;
            });
            QObject::connect(download, &NetJob::progress, this, &JavaDownloader::progress);
            QObject::connect(download, &NetJob::succeeded, [files, isLegacy, this] {
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
                QObject::connect(elementDownload, &NetJob::finished, [elementDownload] { elementDownload->deleteLater(); });
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
            QObject::connect(downloadJob, &NetJob::finished, [downloadJob, metaResponse] {
                downloadJob->deleteLater();
                delete metaResponse;
            });
            QObject::connect(downloadJob, &NetJob::progress, this, &JavaDownloader::progress);
            QObject::connect(downloadJob, &NetJob::succeeded, [metaResponse, isLegacy, this] {
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
                    const QString path = downloadURL.host() + '/' + downloadURL.path();
                    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
                    entry->setStale(true);
                    download->addNetAction(Net::Download::makeCached(downloadURL, entry));
                    auto zippath = entry->getFullPath();
                    QObject::connect(download, &NetJob::finished, [download] { download->deleteLater(); });
                    QObject::connect(download, &NetJob::progress, this, &JavaDownloader::progress);
                    QObject::connect(download, &NetJob::succeeded, [isLegacy, zippath, downloadURL, this] {
                        setStatus(tr("Extracting java"));
                        auto output = FS::PathCombine(FS::PathCombine(QCoreApplication::applicationDirPath(), "java"),
                                                      isLegacy ? "java-legacy" : "java-current");
                        // This should do all of the extracting and creating folders
                        MMCZip::extractDir(zippath, downloadURL.fileName().chopped(4), output);
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
