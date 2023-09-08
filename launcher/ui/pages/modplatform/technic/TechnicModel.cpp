// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2020-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "TechnicModel.h"
#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"

#include "net/ApiDownload.h"
#include "ui/widgets/ProjectItem.h"

#include <QIcon>

Technic::ListModel::ListModel(QObject* parent) : QAbstractListModel(parent) {}

Technic::ListModel::~ListModel() {}

QVariant Technic::ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    Modpack pack = modpacks.at(pos);
    switch (role) {
        case Qt::ToolTipRole: {
            if (pack.description.length() > 100) {
                // some magic to prevent to long tooltips and replace html linebreaks
                QString edit = pack.description.left(97);
                edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
                return edit;
            }
            return pack.description;
        }
        case Qt::DecorationRole: {
            if (m_logoMap.contains(pack.logoName)) {
                return (m_logoMap.value(pack.logoName));
            }
            QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
            ((ListModel*)this)->requestLogo(pack.logoName, pack.logoUrl);
            return icon;
        }
        case Qt::UserRole: {
            QVariant v;
            v.setValue(pack);
            return v;
        }
        case Qt::DisplayRole:
            return pack.name;
        case Qt::SizeHintRole:
            return QSize(0, 58);
        // Custom data
        case UserDataTypes::TITLE:
            return pack.name;
        case UserDataTypes::DESCRIPTION:
            return pack.description;
        case UserDataTypes::SELECTED:
            return false;
        case UserDataTypes::INSTALLED:
            return false;
        default:
            break;
    }

    return {};
}

int Technic::ListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

int Technic::ListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : modpacks.size();
}

void Technic::ListModel::searchWithTerm(const QString& term)
{
    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull()) {
        return;
    }
    currentSearchTerm = term;
    if (hasActiveSearchJob()) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    }

    beginResetModel();
    modpacks.clear();
    endResetModel();
    searchState = None;

    performSearch();
}

void Technic::ListModel::performSearch()
{
    if (hasActiveSearchJob())
        return;

    auto netJob = makeShared<NetJob>("Technic::Search", APPLICATION->network());
    QString searchUrl = "";
    if (currentSearchTerm.isEmpty()) {
        searchUrl = QString("%1trending?build=%2").arg(BuildConfig.TECHNIC_API_BASE_URL, BuildConfig.TECHNIC_API_BUILD);
        searchMode = List;
    } else if (currentSearchTerm.startsWith("http://api.technicpack.net/modpack/")) {
        searchUrl = QString("https://%1?build=%2").arg(currentSearchTerm.mid(7), BuildConfig.TECHNIC_API_BUILD);
        searchMode = Single;
    } else if (currentSearchTerm.startsWith("https://api.technicpack.net/modpack/")) {
        searchUrl = QString("%1?build=%2").arg(currentSearchTerm, BuildConfig.TECHNIC_API_BUILD);
        searchMode = Single;
    } else if (currentSearchTerm.startsWith("#")) {
        searchUrl = QString("https://api.technicpack.net/modpack/%1?build=%2").arg(currentSearchTerm.mid(1), BuildConfig.TECHNIC_API_BUILD);
        searchMode = Single;
    } else {
        searchUrl =
            QString("%1search?build=%2&q=%3").arg(BuildConfig.TECHNIC_API_BASE_URL, BuildConfig.TECHNIC_API_BUILD, currentSearchTerm);
        searchMode = List;
    }
    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(searchUrl), response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob.get(), &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob.get(), &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void Technic::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Technic at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
    }

    QList<Modpack> newList;
    try {
        auto root = Json::requireObject(doc);

        switch (searchMode) {
            case List: {
                auto objs = Json::requireArray(root, "modpacks");
                for (auto technicPack : objs) {
                    Modpack pack;
                    auto technicPackObject = Json::requireObject(technicPack);
                    pack.name = Json::requireString(technicPackObject, "name");
                    pack.slug = Json::requireString(technicPackObject, "slug");
                    if (pack.slug == "vanilla")
                        continue;

                    auto rawURL = Json::ensureString(technicPackObject, "iconUrl", "null");
                    if (rawURL == "null") {
                        pack.logoUrl = "null";
                        pack.logoName = "null";
                    } else {
                        pack.logoUrl = rawURL;
                        pack.logoName = rawURL.section(QLatin1Char('/'), -1);
                    }
                    pack.broken = false;
                    newList.append(pack);
                }
                break;
            }
            case Single: {
                if (root.contains("error")) {
                    // Invalid API url
                    break;
                }

                Modpack pack;
                pack.name = Json::requireString(root, "displayName");
                pack.slug = Json::requireString(root, "name");

                if (root.contains("icon")) {
                    auto iconObj = Json::requireObject(root, "icon");
                    auto iconUrl = Json::requireString(iconObj, "url");

                    pack.logoUrl = iconUrl;
                    pack.logoName = iconUrl.section(QLatin1Char('/'), -1);
                } else {
                    pack.logoUrl = "null";
                    pack.logoName = "null";
                }

                pack.broken = false;
                newList.append(pack);
                break;
            }
        }
    } catch (const JSONValidationError& err) {
        qCritical() << "Couldn't parse technic search results:" << err.cause();
        return;
    }
    searchState = Finished;

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void Technic::ListModel::getLogo(const QString& logo, const QString& logoUrl, Technic::LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()->resolveEntry("TechnicPacks", QString("logos/%1").arg(logo))->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void Technic::ListModel::searchRequestFailed()
{
    jobPtr.reset();

    if (searchState == ResetRequested) {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        performSearch();
    } else {
        searchState = Finished;
    }
}

void Technic::ListModel::logoLoaded(QString logo, QString out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, QIcon(out));
    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].logoName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
        }
    }
}

void Technic::ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void Technic::ListModel::requestLogo(QString logo, QString url)
{
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo) || logo == "null") {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("TechnicPacks", QString("logos/%1").arg(logo));
    auto job = new NetJob(QString("Technic Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::ApiDownload::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();

    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        logoLoaded(logo, fullPath);
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo, job] {
        job->deleteLater();
        logoFailed(logo);
    });

    job->start();

    m_loadingLogos.append(logo);
}
