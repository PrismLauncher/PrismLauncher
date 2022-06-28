#include "ModrinthAPI.h"

#include "Application.h"
#include "Json.h"
#include "net/Upload.h"

auto ModrinthAPI::currentVersion(QString hash, QString hash_format, QByteArray* response) -> NetJob::Ptr
{
    auto* netJob = new NetJob(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    netJob->addNetAction(Net::Download::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hash_format), response));

    QObject::connect(netJob, &NetJob::finished, [response] { delete response; });

    return netJob;
}

auto ModrinthAPI::currentVersions(const QStringList& hashes, QString hash_format, QByteArray* response) -> NetJob::Ptr
{
    auto* netJob = new NetJob(QString("Modrinth::GetCurrentVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files"), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response] { delete response; });

    return netJob;
}

auto ModrinthAPI::latestVersion(QString hash,
                                QString hash_format,
                                std::list<Version> mcVersions,
                                ModLoaderTypes loaders,
                                QByteArray* response) -> NetJob::Ptr
{
    auto* netJob = new NetJob(QString("Modrinth::GetLatestVersion"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "loaders", getModLoaderStrings(loaders));

    QStringList game_versions;
    for (auto& ver : mcVersions) {
        game_versions.append(ver.toString());
    }
    Json::writeStringList(body_obj, "game_versions", game_versions);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1/update?algorithm=%2").arg(hash, hash_format), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response] { delete response; });

    return netJob;
}

auto ModrinthAPI::latestVersions(const QStringList& hashes,
                                 QString hash_format,
                                 std::list<Version> mcVersions,
                                 ModLoaderTypes loaders,
                                 QByteArray* response) -> NetJob::Ptr
{
    auto* netJob = new NetJob(QString("Modrinth::GetLatestVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    Json::writeStringList(body_obj, "loaders", getModLoaderStrings(loaders));

    QStringList game_versions;
    for (auto& ver : mcVersions) {
        game_versions.append(ver.toString());
    }
    Json::writeStringList(body_obj, "game_versions", game_versions);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::Upload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"), response, body_raw));

    QObject::connect(netJob, &NetJob::finished, [response] { delete response; });

    return netJob;
}

auto ModrinthAPI::getProjects(QStringList addonIds, QByteArray* response) const -> NetJob*
{
    auto netJob = new NetJob(QString("Modrinth::GetProjects"), APPLICATION->network());
    auto searchUrl = getMultipleModInfoURL(addonIds);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::finished, [response, netJob] { delete response; netJob->deleteLater(); });

    return netJob;
}
