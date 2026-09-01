// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameAPI.h"
#include <optional>
#include "BuildConfig.h"

#include "Application.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

std::pair<Task::Ptr, QByteArray*> FlameAPI::matchFingerprints(const QList<uint>& fingerprints)
{
    auto netJob = makeShared<NetJob>(QString("Flame::MatchFingerprints"), APPLICATION->network());

    QJsonObject bodyObj;
    QJsonArray fingerprintsArr;
    for (const auto& fp : fingerprints) {
        fingerprintsArr.append(QString("%1").arg(fp));
    }

    bodyObj["fingerprints"] = fingerprintsArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();
    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/fingerprints"), bodyRaw);
    netJob->addNetAction(action);

    return { netJob, response };
}

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

std::pair<Task::Ptr, QByteArray*> FlameAPI::getProjects(QStringList addonIds) const
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetProjects"), APPLICATION->network());

    QJsonObject bodyObj;
    QJsonArray addonsArr;
    for (auto& addonId : addonIds) {
        addonsArr.append(addonId);
    }

    bodyObj["modIds"] = addonsArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();
    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/mods"), bodyRaw);
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [bodyRaw] { qDebug() << bodyRaw; });

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFiles(const QStringList& fileIds)
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

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFile(const QString& addonId, const QString& fileId)
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

namespace {
const auto g_classIDMappings = std::array{
    std::pair{ ModPlatform::ResourceType::Mod, 6 },        std::pair{ ModPlatform::ResourceType::ResourcePack, 12 },
    std::pair{ ModPlatform::ResourceType::World, 17 },     std::pair{ ModPlatform::ResourceType::ShaderPack, 6552 },
    std::pair{ ModPlatform::ResourceType::Modpack, 4471 }, std::pair{ ModPlatform::ResourceType::DataPack, 6945 },
};
}

int FlameAPI::getClassId(ModPlatform::ResourceType type)
{
    for (auto&& [e, classId] : g_classIDMappings) {
        if (e == type) {
            return classId;
        }
    }
    return 0;
}
ModPlatform::ResourceType FlameAPI::getResourceType(int classId)
{
    for (auto&& [type, c] : g_classIDMappings) {
        if (c == classId) {
            return type;
        }
    }
    return ModPlatform::ResourceType::Unknown;
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getCategories(ModPlatform::ResourceType type)
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetCategories"), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(
        QUrl(QString(BuildConfig.FLAME_BASE_URL + "/categories?gameId=432&classId=%1").arg(getClassId(type))));
    netJob->addNetAction(action);
    QObject::connect(netJob.get(), &Task::failed, netJob.get(),
                     [](const QString& msg) { qDebug() << "Flame failed to get categories:" << msg; });
    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getModCategories() const
{
    return getCategories(ModPlatform::ResourceType::Mod);
}

QList<ModPlatform::Category> FlameAPI::loadModCategories(const QByteArray& response) const
{
    QList<ModPlatform::Category> categories;
    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from categories at" << parseError.offset << "reason:" << parseError.errorString();
        qWarning() << *response;
        return categories;
    }

    try {
        auto obj = Json::requireObject(doc);
        auto arr = Json::requireArray(obj, "data");

        for (auto val : arr) {
            auto cat = Json::requireObject(val);
            auto id = Json::requireInteger(cat, "id");
            auto name = Json::requireString(cat, "name");
            categories.push_back({ name, QString::number(id) });
        }

    } catch (Json::JsonException& e) {
        qCritical() << "Failed to parse response from a version request.";
        qCritical() << e.what();
        qDebug() << doc;
    }
    return categories;
};

std::optional<ModPlatform::IndexedVersion> FlameAPI::getLatestVersion(const QList<ModPlatform::IndexedVersion>& versions,
                                                                      const QList<ModPlatform::ModLoaderType>& instanceLoaders,
                                                                      ModPlatform::ModLoaderTypes fallback,
                                                                      bool checkLoaders,
                                                                      std::vector<ModPlatform::IndexedVersionType> releaseTypes)
{
    static const auto s_noLoader = ModPlatform::ModLoaderType(0);
    if (!checkLoaders) {
        std::optional<ModPlatform::IndexedVersion> ver;
        for (const auto& fileTmp : versions) {
            if (!releaseTypes.empty() &&
                std::find(releaseTypes.cbegin(), releaseTypes.cend(), fileTmp.versionType) == releaseTypes.cend()) {
                continue;
            }
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
        if (!releaseTypes.empty() && std::find(releaseTypes.cbegin(), releaseTypes.cend(), fileTmp.versionType) == releaseTypes.cend()) {
            continue;
        }
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
    auto currentLoaders = instanceLoaders + ModPlatform::modLoaderTypesToList(fallback);
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
