#include "JavaDownloader.h"
#include "Application.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "quazip.h"

// Quick & dirty struct to store files
struct File {
    QString path;
    QString url;
    QByteArray hash;
};

void JavaDownloader::downloadJava(bool isLegacy, const QString& OS)
{
    auto netJob = new NetJob(QString("JRE::QueryVersions"), APPLICATION->network());
    auto response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(
        QUrl("https://piston-meta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"), response));
    QObject::connect(netJob, &NetJob::finished, [netJob, response] {
        netJob->deleteLater();
        delete response;
    });
    QObject::connect(netJob, &NetJob::succeeded, [response, &OS, isLegacy] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }
        auto versionArray = doc.object()[OS].toObject()[isLegacy ? "jre-legacy" : "java-runtime-gamma"].toArray();
        if (!versionArray.empty()) {
            auto url = versionArray[0].toObject()["manifest"].toObject()["url"].toString();
            auto download = new NetJob(QString("JRE::DownloadJava"), APPLICATION->network());
            auto files = new QByteArray();

            download->addNetAction(Net::Download::makeByteArray(QUrl(url), files));

            QObject::connect(download, &NetJob::finished, [download, files] {
                download->deleteLater();
                delete files;
            });
            QObject::connect(download, &NetJob::succeeded, [files, isLegacy] {
                QJsonParseError parse_error{};
                QJsonDocument doc = QJsonDocument::fromJson(*files, &parse_error);
                if (parse_error.error != QJsonParseError::NoError) {
                    qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
                    qWarning() << *files;
                    return;
                }

                // valid json doc, begin making jre spot
                auto output =
                    FS::PathCombine(QCoreApplication::applicationDirPath(), QString("java/") + (isLegacy ? "java-legacy" : "java-current"));
                FS::ensureFolderPathExists(output);
                std::vector<File> toDownload;
                auto list = doc.object()["files"].toObject();
                for (auto element : list) {
                    auto obj = element.toObject();
                    for (const auto& paths : obj.keys()) {
                        auto file = FS::PathCombine(output, paths);

                        auto type = obj[paths].toObject()["type"].toString();
                        if (type == "directory") {
                            FS::ensureFolderPathExists(file);
                        } else if (type == "link") {
                            // this is linux only !
                            auto target = FS::PathCombine(file, "../" + obj[paths].toObject()["target"].toString());
                            QFile(target).link(file);
                        } else if (type == "file") {
                            // TODO download compressed version if it exists ?
                            auto raw = obj[paths].toObject()["downloads"].toObject()["raw"].toObject();
                            auto f = File{ file, raw["url"].toString(), QByteArray::fromHex(raw["sha1"].toString().toLatin1()) };
                            toDownload.push_back(f);
                        }
                    }
                }
                auto elementDownload = new NetJob("JRE::FileDownload", APPLICATION->network());
                for (const auto& file : toDownload) {
                    auto dl = Net::Download::makeFile(file.url, file.path);
                    dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.hash));
                    elementDownload->addNetAction(dl);
                }
                QObject::connect(elementDownload, &NetJob::finished, [elementDownload] { elementDownload->deleteLater(); });
                elementDownload->start();
            });
            download->start();
        } else {
            // mojang does not have a JRE for us, let's get azul zulu
            QString javaVersion = isLegacy ? QString("8.0") : QString("17.0");
            QString azulOS;
            QString arch;
            QString bitness;

            if (OS == "mac-os-arm64") {
                // macos arm64
                azulOS = "macos";
                arch = "arm";
                bitness = "64";
            } else if (OS == "linux-aarch64") {
                // linux aarch64
                azulOS = "linux";
                arch = "arm";
                bitness = "64";
            }
            auto metaResponse = new QByteArray();
            auto downloadJob = new NetJob(QString("JRE::QueryAzulMeta"), APPLICATION->network());
            downloadJob->addNetAction(Net::Download::makeByteArray(QString("https://api.azul.com/zulu/download/community/v1.0/bundles/?"
                                                                           "java_version=%1"
                                                                           "&os=%2"
                                                                           "&arch=%3"
                                                                           "&hw_bitness=%4"
                                                                           "&ext=zip"          // as a zip for all os, even linux
                                                                           "&bundle_type=jre"  // jre only
                                                                           "&latest=true"      // only get the one latest entry
                                                                           )
                                                                       .arg(javaVersion, azulOS, arch, bitness),
                                                                   metaResponse));
            QObject::connect(downloadJob, &NetJob::finished, [downloadJob, metaResponse] {
                downloadJob->deleteLater();
                delete metaResponse;
            });

            QObject::connect(downloadJob, &NetJob::succeeded, [metaResponse, isLegacy] {
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
                    auto downloadURL = QUrl(array[0].toObject()["url"].toString());
                    auto download = new NetJob(QString("JRE::DownloadJava"), APPLICATION->network());
                    const QString path = downloadURL.host() + '/' + downloadURL.path();
                    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
                    entry->setStale(true);
                    download->addNetAction(Net::Download::makeCached(downloadURL, entry));
                    auto zippath = entry->getFullPath();
                    QObject::connect(download, &NetJob::finished, [download] { download->deleteLater(); });
                    QObject::connect(download, &NetJob::succeeded, [isLegacy, zippath] {
                        auto output = FS::PathCombine(FS::PathCombine(QCoreApplication::applicationDirPath(), "java"),
                                                      isLegacy ? "java-legacy" : "java-current");
                        // This should do all of the extracting and creating folders
                        MMCZip::extractDir(zippath, output);
                    });
                } else {
                    qWarning() << "No suitable JRE found !!";
                }
            });
        }
    });

    netJob->start();
}