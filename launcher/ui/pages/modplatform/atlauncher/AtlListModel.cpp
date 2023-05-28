/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AtlListModel.h"

#include <Application.h>
#include <BuildConfig.h>
#include <Json.h>

namespace Atl {

ListModel::ListModel(QObject* parent) : QAbstractListModel(parent) {}

ListModel::~ListModel() {}

int ListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : modpacks.size();
}

int ListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    ATLauncher::IndexedPack pack = modpacks.at(pos);
    if (role == Qt::DisplayRole) {
        return pack.name;
    } else if (role == Qt::ToolTipRole) {
        return pack.name;
    } else if (role == Qt::DecorationRole) {
        if (m_logoMap.contains(pack.safeName)) {
            return (m_logoMap.value(pack.safeName));
        }
        auto icon = APPLICATION->getThemedIcon("atlauncher-placeholder");

        auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/%1.png").arg(pack.safeName.toLower());
        ((ListModel*)this)->requestLogo(pack.safeName, url);

        return icon;
    } else if (role == Qt::UserRole) {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return QVariant();
}

void ListModel::request()
{
    beginResetModel();
    modpacks.clear();
    endResetModel();

    auto netJob = makeShared<NetJob>("Atl::Request", APPLICATION->network());
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/json/packsnew.json");
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(url), response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob.get(), &NetJob::succeeded, this, &ListModel::requestFinished);
    QObject::connect(netJob.get(), &NetJob::failed, this, &ListModel::requestFailed);
}

void ListModel::requestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ATL at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
    }

    QList<ATLauncher::IndexedPack> newList;

    auto packs = doc.array();
    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ATLauncher::IndexedPack pack;

        try {
            ATLauncher::loadIndexedPack(pack, packObj);
        } catch (const JSONValidationError& e) {
            qDebug() << QString::fromUtf8(*response);
            qWarning() << "Error while reading pack manifest from ATLauncher: " << e.cause();
            return;
        }

        // ignore packs without a published version
        if (pack.versions.length() == 0)
            continue;
        // only display public packs (for now)
        if (pack.type != ATLauncher::PackType::Public)
            continue;
        // ignore "system" packs (Vanilla, Vanilla with Forge, etc)
        if (pack.system)
            continue;

        newList.append(pack);
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void ListModel::requestFailed(QString reason)
{
    jobPtr.reset();
}

void ListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(
            APPLICATION->metacache()->resolveEntry("ATLauncherPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);

    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].safeName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
        }
    }
}

void ListModel::requestLogo(QString file, QString url)
{
    if (m_loadingLogos.contains(file) || m_failedLogos.contains(file)) {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("ATLauncherPacks", QString("logos/%1").arg(file.section(".", 0, 0)));
    auto job = new NetJob(QString("ATLauncher Icon Download %1").arg(file), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, file, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(file, QIcon(fullPath));
        if (waitingCallbacks.contains(file)) {
            waitingCallbacks.value(file)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, file, job] {
        job->deleteLater();
        emit logoFailed(file);
    });

    job->start();

    m_loadingLogos.append(file);
}

}  // namespace Atl
