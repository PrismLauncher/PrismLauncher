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

#include "FtbListModel.h"

#include "BuildConfig.h"
#include "Application.h"
#include "Json.h"

#include <QPainter>

namespace Ftb {

ListModel::ListModel(QObject *parent) : QAbstractListModel(parent)
{
}

ListModel::~ListModel()
{
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    return modpacks.size();
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QString("INVALID INDEX %1").arg(pos);
    }

    ModpacksCH::Modpack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if (role == Qt::ToolTipRole)
    {
        return pack.synopsis;
    }
    else if(role == Qt::DecorationRole)
    {
        QIcon placeholder = APPLICATION->getThemedIcon("screenshot-placeholder");

        auto iter = m_logoMap.find(pack.name);
        if (iter != m_logoMap.end()) {
            auto & logo = *iter;
            if(!logo.result.isNull()) {
                return logo.result;
            }
            return placeholder;
        }

        for(auto art : pack.art) {
            if(art.type == "square") {
                ((ListModel *)this)->requestLogo(pack.name, art.url);
            }
        }
        return placeholder;
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return QVariant();
}

void ListModel::getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(APPLICATION->metacache()->resolveEntry("ModpacksCHPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    }
    else
    {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::request()
{
    m_aborted = false;

    beginResetModel();
    modpacks.clear();
    endResetModel();

    auto *netJob = new NetJob("Ftb::Request", APPLICATION->network());
    auto url = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/all");
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(url), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::requestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::requestFailed);
}

void ListModel::abortRequest()
{
    m_aborted = jobPtr->abort();
    jobPtr.reset();
}

void ListModel::requestFinished()
{
    jobPtr.reset();
    remainingPacks.clear();

    QJsonParseError parse_error {};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ModpacksCH at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto packs = doc.object().value("packs").toArray();
    for(auto pack : packs) {
        auto packId = pack.toInt();
        remainingPacks.append(packId);
    }

    if(!remainingPacks.isEmpty()) {
        currentPack = remainingPacks.at(0);
        requestPack();
    }
}

void ListModel::requestFailed(QString reason)
{
    jobPtr.reset();
    remainingPacks.clear();
}

void ListModel::requestPack()
{
    auto *netJob = new NetJob("Ftb::Search", APPLICATION->network());
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1").arg(currentPack);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::packRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::packRequestFailed);
}

void ListModel::packRequestFinished()
{
    if (!jobPtr || m_aborted)
        return;

    jobPtr.reset();
    remainingPacks.removeOne(currentPack);

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);

    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ModpacksCH at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ModpacksCH::Modpack pack;
    try
    {
        ModpacksCH::loadModpack(pack, obj);
    }
    catch (const JSONValidationError &e)
    {
        qDebug() << QString::fromUtf8(response);
        qWarning() << "Error while reading pack manifest from ModpacksCH: " << e.cause();
        return;
    }

    // Since there is no guarantee that packs have a version, this will just
    // ignore those "dud" packs.
    if (pack.versions.empty())
    {
        qWarning() << "ModpacksCH Pack " << pack.id << " ignored. reason: lacking any versions";
    }
    else
    {
        beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size());
        modpacks.append(pack);
        endInsertRows();
    }

    if(!remainingPacks.isEmpty()) {
        currentPack = remainingPacks.at(0);
        requestPack();
    }
}

void ListModel::packRequestFailed(QString reason)
{
    jobPtr.reset();
    remainingPacks.removeOne(currentPack);
}

void ListModel::logoLoaded(QString logo, bool stale)
{
    auto & logoObj = m_logoMap[logo];
    logoObj.downloadJob.reset();
    QString smallPath = logoObj.fullpath + ".small";

    QFileInfo smallInfo(smallPath);

    if(stale || !smallInfo.exists()) {
        QImage image(logoObj.fullpath);
        if (image.isNull())
        {
            logoObj.failed = true;
            return;
        }
        QImage small;
        if (image.width() > image.height()) {
            small = image.scaledToWidth(512).scaledToWidth(256, Qt::SmoothTransformation);
        }
        else {
            small = image.scaledToHeight(512).scaledToHeight(256, Qt::SmoothTransformation);
        }
        QPoint offset((256 - small.width()) / 2, (256 - small.height()) / 2);
        QImage square(QSize(256, 256), QImage::Format_ARGB32);
        square.fill(Qt::transparent);

        QPainter painter(&square);
        painter.drawImage(offset, small);
        painter.end();

        square.save(logoObj.fullpath + ".small", "PNG");
    }

    logoObj.result = QIcon(logoObj.fullpath + ".small");
    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].name == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
        }
    }
}

void ListModel::logoFailed(QString logo)
{
    m_logoMap[logo].failed = true;
    m_logoMap[logo].downloadJob.reset();
}

void ListModel::requestLogo(QString logo, QString url)
{
    if(m_logoMap.contains(logo)) {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("ModpacksCHPacks", QString("logos/%1").arg(logo.section(".", 0, 0)));

    bool stale = entry->isStale();

    NetJob *job = new NetJob(QString("ModpacksCH Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::finished, this, [this, logo, fullPath, stale]
    {
        logoLoaded(logo, stale);
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo]
    {
        logoFailed(logo);
    });

    auto &newLogoEntry = m_logoMap[logo];
    newLogoEntry.downloadJob = job;
    newLogoEntry.fullpath = fullPath;
    job->start();
}

}
