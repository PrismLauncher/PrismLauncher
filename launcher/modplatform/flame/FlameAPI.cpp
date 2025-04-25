// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameAPI.h"
#include <memory>
#include <optional>

#include "Application.h"
#include "Json.h"
#include "api/Api.h"
#include "api/structures/ModLoader.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/NetJob.h"

QString FlameAPI::getModFileChangelog(int modId, int fileId)
{
    QEventLoop lock;
    QString changelog;

    auto netJob = makeShared<NetJob>(QString("Flame::FileChangelog"), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();
    netJob->addNetAction(Net::ApiDownload::makeByteArray(
        QString("https://api.curseforge.com/v1/mods/%1/files/%2/changelog")
            .arg(QString::fromStdString(std::to_string(modId)), QString::fromStdString(std::to_string(fileId))),
        response));

    QObject::connect(netJob.get(), &NetJob::succeeded, [&netJob, response, &changelog] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame::FileChangelog at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            netJob->failed(parse_error.errorString());
            return;
        }

        changelog = Json::ensureString(doc.object(), "data");
    });

    QObject::connect(netJob.get(), &NetJob::finished, [&lock] { lock.quit(); });

    netJob->start();
    lock.exec();

    return changelog;
}

NetJob::Ptr FlameAPI::getFiles(const QStringList& fileIds, std::shared_ptr<QByteArray> response) const
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFiles"), APPLICATION->network());

    QJsonObject body_obj;
    QJsonArray files_arr;
    for (auto& fileId : fileIds) {
        files_arr.append(fileId);
    }

    body_obj["fileIds"] = files_arr;

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::ApiUpload::makeByteArray(QString("https://api.curseforge.com/v1/mods/files"), response, body_raw));

    QObject::connect(netJob.get(), &NetJob::failed, [body_raw] { qDebug() << body_raw; });

    return netJob;
}

Task::Ptr FlameAPI::getFile(const QString& addonId, const QString& fileId, std::shared_ptr<QByteArray> response) const
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFile"), APPLICATION->network());
    netJob->addNetAction(
        Net::ApiDownload::makeByteArray(QUrl(QString("https://api.curseforge.com/v1/mods/%1/files/%2").arg(addonId, fileId)), response));

    QObject::connect(netJob.get(), &NetJob::failed, [addonId, fileId] { qDebug() << "Flame API file failure" << addonId << fileId; });

    return netJob;
}

std::optional<Platform::Version> FlameAPI::getLatestVersion(QList<Platform::Version> versions,
                                                            QList<Platform::ModLoader> instanceLoaders,
                                                            Platform::ModLoaders modLoaders)
{
    static const auto noLoader = Platform::ModLoader(0);
    QHash<Platform::ModLoader, Platform::Version> bestMatch;
    auto checkVersion = [&bestMatch](const Platform::Version& version, const Platform::ModLoader& loader) {
        if (bestMatch.contains(loader)) {
            auto best = bestMatch.value(loader);
            if (version.date > best.date) {
                bestMatch[loader] = version;
            }
        } else {
            bestMatch[loader] = version;
        }
    };
    for (auto file_tmp : versions) {
        auto loaders = Platform::ModloaderUtils::toList(file_tmp.loaders);
        if (loaders.isEmpty()) {
            checkVersion(file_tmp, noLoader);
        } else {
            for (auto loader : loaders) {
                checkVersion(file_tmp, loader);
            }
        }
    }
    // edge case: mod has installed for forge but the instance is fabric => fabric version will be prioritizated on update
    auto currentLoaders = instanceLoaders + Platform::ModloaderUtils::toList(modLoaders);
    currentLoaders.append(noLoader);  // add a fallback in case the versions do not define a loader

    for (auto loader : currentLoaders) {
        if (bestMatch.contains(loader)) {
            auto bestForLoader = bestMatch.value(loader);
            // awkward case where the mod has only two loaders and one of them is not specified
            if (loader != noLoader && bestMatch.contains(noLoader) && bestMatch.size() == 2) {
                auto bestForNoLoader = bestMatch.value(noLoader);
                if (bestForNoLoader.date > bestForLoader.date) {
                    return bestForNoLoader;
                }
            }
            return bestForLoader;
        }
    }
    return {};
}
