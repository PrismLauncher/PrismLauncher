// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameAPI.h"
#include <memory>
#include <optional>
#include "BuildConfig.h"

#include "Application.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

QString FlameAPI::getModFileChangelog(int modId, int fileId)
{
    QEventLoop lock;
    QString changelog;

    auto netJob = makeShared<NetJob>(QString("Flame::FileChangelog"), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(
        QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2/changelog")
            .arg(QString::fromStdString(std::to_string(modId)), QString::fromStdString(std::to_string(fileId))));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [&netJob, response, &changelog] {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame::FileChangelog at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *response;

            netJob->failed(parseError.errorString());
            return;
        }

        changelog = doc.object()["data"].toString();
    });

    QObject::connect(netJob.get(), &NetJob::finished, &lock, &QEventLoop::quit);

    netJob->start();
    lock.exec();

    return changelog;
}

QString FlameAPI::getModDescription(int modId)
{
    QEventLoop lock;
    QString description;

    auto netJob = makeShared<NetJob>(QString("Flame::ModDescription"), APPLICATION->network());
    auto [action, response] =
        Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/description").arg(QString::number(modId)));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [&netJob, response, &description] {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame::ModDescription at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *response;

            netJob->failed(parseError.errorString());
            return;
        }

        description = doc.object()["data"].toString();
    });

    QObject::connect(netJob.get(), &NetJob::finished, &lock, &QEventLoop::quit);

    netJob->start();
    lock.exec();

    return description;
}

Net::Spec<QList<ModPlatform::IndexedPack::Ptr>> FlameAPI::getProjects(QStringList addonIds) const
{
    QJsonObject bodyObj;
    QJsonArray addonsArr;
    for (auto& addonId : addonIds) {
        addonsArr.append(addonId);
    }

    bodyObj["modIds"] = addonsArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto parseFunc = [this](const QByteArray& response) -> Net::RpcSink<QList<ModPlatform::IndexedPack::Ptr>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from CurseForge projects task at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QList<ModPlatform::IndexedPack::Ptr> projects;
        try {
            auto entries = Json::requireArray(Json::requireObject(doc), "data");
            for (auto entry : entries) {
                auto pack = std::make_shared<ModPlatform::IndexedPack>();
                auto entryObj = Json::requireObject(entry);
                loadIndexedPack(*pack, entryObj);
                projects.append(pack);
            }
        } catch (Json::JsonException& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource info:" << e.cause();
            return std::unexpected(e.cause());
        }
        return projects;
    };

    return Net::Spec<QList<ModPlatform::IndexedPack::Ptr>>{ .method = Net::NetRequest::HttpMethod::Post,
                                                            .url = QUrl(BuildConfig.FLAME_BASE_URL + "/mods"),
                                                            .data = bodyRaw,
                                                            .parse = parseFunc,
                                                            .name = "Flame::GetProjects" };
}

Net::Spec<QList<FlameMod::FingerprintMatch>> FlameAPI::matchFingerprints(const QList<uint>& fingerprints)
{
    QJsonObject bodyObj;
    QJsonArray fingerprintsArr;
    for (const auto& fp : fingerprints) {
        fingerprintsArr.append(QString("%1").arg(fp));
    }

    bodyObj["fingerprints"] = fingerprintsArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto parseFunc = [](const QByteArray& response) -> Net::RpcSink<QList<FlameMod::FingerprintMatch>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame::MatchFingerprints at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QList<FlameMod::FingerprintMatch> matches;
        try {
            auto docObj = Json::requireObject(doc);
            auto dataObj = Json::requireObject(docObj, "data");
            auto dataArr = Json::requireArray(dataObj, "exactMatches");

            for (auto match : dataArr) {
                auto matchObj = match.toObject();
                auto fileObj = matchObj["file"].toObject();

                if (matchObj.isEmpty() || fileObj.isEmpty()) {
                    qWarning() << "Fingerprint match is empty!";
                    continue;
                }

                FlameMod::FingerprintMatch fm{};
                fm.fileFingerprint = fileObj["fileFingerprint"].toVariant().toLongLong();
                fm.modId = fileObj["modId"].toVariant().toLongLong();
                fm.fileId = fileObj["id"].toVariant().toLongLong();
                fm.isAvailable = fileObj["isAvailable"].toBool();
                matches.append(fm);
            }
        } catch (Json::JsonException& e) {
            qDebug() << doc;
            qWarning() << "Error while reading Flame fingerprint matches:" << e.cause();
            return std::unexpected(e.cause());
        }
        return matches;
    };

    return Net::Spec<QList<FlameMod::FingerprintMatch>>{ .method = Net::NetRequest::HttpMethod::Post,
                                                         .url = QUrl(BuildConfig.FLAME_BASE_URL + "/fingerprints"),
                                                         .data = bodyRaw,
                                                         .parse = parseFunc,
                                                         .name = "Flame::MatchFingerprints" };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFiles(const QStringList& fileIds) const
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFiles"), APPLICATION->network());

    QJsonObject bodyObj;
    QJsonArray filesArr;
    for (const auto& fileId : fileIds) {
        filesArr.append(fileId);
    }

    bodyObj["fileIds"] = filesArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/mods/files"), bodyRaw);
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [bodyRaw] { qDebug() << bodyRaw; });

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFile(const QString& addonId, const QString& fileId) const
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFile"), APPLICATION->network());
    auto [action, response] =
        Net::ApiRequest::makeByteArray(QUrl(QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2").arg(addonId, fileId)));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(),
                     [addonId, fileId] { qDebug() << "Flame API file failure" << addonId << fileId; });

    return { netJob, response };
}

QList<ResourceAPI::SortingMethod> FlameAPI::getSortingMethods() const
{
    // https://docs.curseforge.com/?python#tocS_ModsSearchSortField
    return { { .index = 1, .name = "Featured", .readableName = QObject::tr("Sort by Featured") },
             { .index = 2, .name = "Popularity", .readableName = QObject::tr("Sort by Popularity") },
             { .index = 3, .name = "LastUpdated", .readableName = QObject::tr("Sort by Last Updated") },
             { .index = 4, .name = "Name", .readableName = QObject::tr("Sort by Name") },
             { .index = 5, .name = "Author", .readableName = QObject::tr("Sort by Author") },
             { .index = 6, .name = "TotalDownloads", .readableName = QObject::tr("Sort by Downloads") },
             { .index = 7, .name = "Category", .readableName = QObject::tr("Sort by Category") },
             { .index = 8, .name = "GameVersion", .readableName = QObject::tr("Sort by Game Version") } };
}

Net::Spec<QList<ModPlatform::Category>> FlameAPI::getCategories(ModPlatform::ResourceType type) const
{
    auto parseFunc = [](const QByteArray& response) -> Net::RpcSink<QList<ModPlatform::Category>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from categories at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QList<ModPlatform::Category> categories;
        try {
            auto obj = Json::requireObject(doc);
            auto arr = Json::requireArray(obj, "data");

            for (auto val : arr) {
                auto cat = Json::requireObject(val);
                auto id = Json::requireInteger(cat, "id");
                auto name = Json::requireString(cat, "name");
                categories.push_back({ .name = name, .id = QString::number(id) });
            }

        } catch (Json::JsonException& e) {
            qCritical() << "Failed to parse response from a version request.";
            qCritical() << e.what();
            qDebug() << doc;
            return std::unexpected(e.what());
        }
        return categories;
    };

    return Net::Spec<QList<ModPlatform::Category>>{
        .url = QUrl(QString(BuildConfig.FLAME_BASE_URL + "/categories?gameId=432&classId=%1").arg(getClassId(type))),
        .parse = parseFunc,
        .name = "FlameAPI::getCategories"
    };
}

std::optional<ModPlatform::IndexedVersion> FlameAPI::getLatestVersion(const QList<ModPlatform::IndexedVersion>& versions,
                                                                      const QList<ModPlatform::ModLoaderType>& instanceLoaders,
                                                                      ModPlatform::ModLoaderTypes modLoaders,
                                                                      bool checkLoaders)
{
    static const auto s_noLoader = ModPlatform::ModLoaderType(0);
    if (!checkLoaders) {
        std::optional<ModPlatform::IndexedVersion> ver;
        for (const auto& fileTmp : versions) {
            if (!ver.has_value() || fileTmp.date > ver->date) {
                ver = fileTmp;
            }
        }
        return ver;
    }
    QHash<ModPlatform::ModLoaderType, ModPlatform::IndexedVersion> bestMatch;
    auto checkVersion = [&bestMatch](const ModPlatform::IndexedVersion& version, const ModPlatform::ModLoaderType& loader) {
        if (bestMatch.contains(loader)) {
            auto best = bestMatch.value(loader);
            if (version.date > best.date) {
                bestMatch[loader] = version;
            }
        } else {
            bestMatch[loader] = version;
        }
    };
    for (const auto& fileTmp : versions) {
        auto loaders = ModPlatform::modLoaderTypesToList(fileTmp.loaders);
        if (loaders.isEmpty()) {
            checkVersion(fileTmp, s_noLoader);
        } else {
            for (auto loader : loaders) {
                checkVersion(fileTmp, loader);
            }
        }
    }
    // edge case: mod has installed for forge but the instance is fabric => fabric version will be prioritizated on update
    auto currentLoaders = instanceLoaders + ModPlatform::modLoaderTypesToList(modLoaders);
    currentLoaders.append(s_noLoader);  // add a fallback in case the versions do not define a loader

    for (auto loader : currentLoaders) {
        if (bestMatch.contains(loader)) {
            auto bestForLoader = bestMatch.value(loader);
            // awkward case where the mod has only two loaders and one of them is not specified
            if (loader != s_noLoader && bestMatch.contains(s_noLoader) && bestMatch.size() == 2) {
                auto bestForNoLoader = bestMatch.value(s_noLoader);
                if (bestForNoLoader.date > bestForLoader.date) {
                    return bestForNoLoader;
                }
            }
            return bestForLoader;
        }
    }
    return {};
}
