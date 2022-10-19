#include "FlameAPI.h"
#include "FlameModIndex.h"

#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"

#include "net/Upload.h"

auto FlameAPI::matchFingerprints(const QList<uint>& fingerprints, QByteArray* response) -> NetJob::Ptr
{
    auto* netJob = new NetJob(QString("Flame::MatchFingerprints"), APPLICATION->network());

    QJsonObject body_obj;
    QJsonArray fingerprints_arr;
    for (auto& fp : fingerprints) {
        fingerprints_arr.append(QString("%1").arg(fp));
    }

    body_obj["fingerprints"] = fingerprints_arr;

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(QString("https://api.curseforge.com/v1/fingerprints"), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response] { delete response; });

    return netJob;
}

auto FlameAPI::getModFileChangelog(int modId, int fileId) -> QString
{
    QEventLoop lock;
    QString changelog;

    auto* netJob = new NetJob(QString("Flame::FileChangelog"), APPLICATION->network());
    auto* response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(
        QString("https://api.curseforge.com/v1/mods/%1/files/%2/changelog")
            .arg(QString::fromStdString(std::to_string(modId)), QString::fromStdString(std::to_string(fileId))),
        response));

    QObject::connect(netJob, &NetJob::succeeded, [netJob, response, &changelog] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qCWarning(LAUNCHER_LOG) << "Error while parsing JSON response from Flame::FileChangelog at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qCWarning(LAUNCHER_LOG) << *response;

            netJob->failed(parse_error.errorString());
            return;
        }

        changelog = Json::ensureString(doc.object(), "data");
    });

    QObject::connect(netJob, &NetJob::finished, [response, &lock] {
        delete response;
        lock.quit();
    });

    netJob->start();
    lock.exec();

    return changelog;
}

auto FlameAPI::getModDescription(int modId) -> QString
{
    QEventLoop lock;
    QString description;

    auto* netJob = new NetJob(QString("Flame::ModDescription"), APPLICATION->network());
    auto* response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(
        QString("https://api.curseforge.com/v1/mods/%1/description")
            .arg(QString::number(modId)), response));

    QObject::connect(netJob, &NetJob::succeeded, [netJob, response, &description] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qCWarning(LAUNCHER_LOG) << "Error while parsing JSON response from Flame::ModDescription at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qCWarning(LAUNCHER_LOG) << *response;

            netJob->failed(parse_error.errorString());
            return;
        }

        description = Json::ensureString(doc.object(), "data");
    });

    QObject::connect(netJob, &NetJob::finished, [response, &lock] {
        delete response;
        lock.quit();
    });

    netJob->start();
    lock.exec();

    return description;
}

auto FlameAPI::getLatestVersion(VersionSearchArgs&& args) -> ModPlatform::IndexedVersion
{
    QEventLoop loop;

    auto netJob = new NetJob(QString("Flame::GetLatestVersion(%1)").arg(args.addonId), APPLICATION->network());
    auto response = new QByteArray();
    ModPlatform::IndexedVersion ver;

    netJob->addNetAction(Net::Download::makeByteArray(getVersionsURL(args), response));

    QObject::connect(netJob, &NetJob::succeeded, [response, args, &ver] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qCWarning(LAUNCHER_LOG) << "Error while parsing JSON response from latest mod version at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qCWarning(LAUNCHER_LOG) << *response;
            return;
        }

        try {
            auto obj = Json::requireObject(doc);
            auto arr = Json::requireArray(obj, "data");

            QJsonObject latest_file_obj;
            ModPlatform::IndexedVersion ver_tmp;

            for (auto file : arr) {
                auto file_obj = Json::requireObject(file);
                auto file_tmp = FlameMod::loadIndexedPackVersion(file_obj);
                if(file_tmp.date > ver_tmp.date) {
                    ver_tmp = file_tmp;
                    latest_file_obj = file_obj;
                }
            }

            ver = FlameMod::loadIndexedPackVersion(latest_file_obj);
        } catch (Json::JsonException& e) {
            qCCritical(LAUNCHER_LOG) << "Failed to parse response from a version request.";
            qCCritical(LAUNCHER_LOG) << e.what();
            qCDebug(LAUNCHER_LOG) << doc;
        }
    });

    QObject::connect(netJob, &NetJob::finished, [response, netJob, &loop] {
        netJob->deleteLater();
        delete response;
        loop.quit();
    });

    netJob->start();

    loop.exec();

    return ver;
}

auto FlameAPI::getProjects(QStringList addonIds, QByteArray* response) const -> NetJob*
{
    auto* netJob = new NetJob(QString("Flame::GetProjects"), APPLICATION->network());

    QJsonObject body_obj;
    QJsonArray addons_arr;
    for (auto& addonId : addonIds) {
        addons_arr.append(addonId);
    }

    body_obj["modIds"] = addons_arr;

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(QString("https://api.curseforge.com/v1/mods"), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response, netJob] { delete response; netJob->deleteLater(); });
    QObject::connect(netJob, &NetJob::failed, [body_raw] { qCDebug(LAUNCHER_LOG) << body_raw; });

    return netJob;
}

auto FlameAPI::getFiles(const QStringList& fileIds, QByteArray* response) const -> NetJob*
{
    auto* netJob = new NetJob(QString("Flame::GetFiles"), APPLICATION->network());

    QJsonObject body_obj;
    QJsonArray files_arr;
    for (auto& fileId : fileIds) {
        files_arr.append(fileId);
    }

    body_obj["fileIds"] = files_arr;

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(QString("https://api.curseforge.com/v1/mods/files"), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response, netJob] { delete response; netJob->deleteLater(); });
    QObject::connect(netJob, &NetJob::failed, [body_raw] { qCDebug(LAUNCHER_LOG) << body_raw; });

    return netJob;
}
