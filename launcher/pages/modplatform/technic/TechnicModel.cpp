/* Copyright 2020-2021 MultiMC Contributors
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

#include "TechnicModel.h"
#include "Application.h"
#include "Json.h"

#include <QIcon>

Technic::ListModel::ListModel(QObject *parent) : QAbstractListModel(parent)
{
}

Technic::ListModel::~ListModel()
{
}

QVariant Technic::ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QString("INVALID INDEX %1").arg(pos);
    }

    Modpack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if(role == Qt::DecorationRole)
    {
        if(m_logoMap.contains(pack.logoName))
        {
            return (m_logoMap.value(pack.logoName));
        }
        QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
        ((ListModel *)this)->requestLogo(pack.logoName, pack.logoUrl);
        return icon;
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }
    return QVariant();
}

int Technic::ListModel::columnCount(const QModelIndex&) const
{
    return 1;
}

int Technic::ListModel::rowCount(const QModelIndex&) const
{
    return modpacks.size();
}

void Technic::ListModel::searchWithTerm(const QString& term)
{
    if(currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull()) {
        return;
    }
    currentSearchTerm = term;
    if(jobPtr) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    }
    else {
        beginResetModel();
        modpacks.clear();
        endResetModel();
        searchState = None;
    }
    performSearch();
}

void Technic::ListModel::performSearch()
{
    NetJob *netJob = new NetJob("Technic::Search");
    QString searchUrl = "";
    if (currentSearchTerm.isEmpty()) {
        searchUrl = "https://api.technicpack.net/trending?build=multimc";
    }
    else
    {
        searchUrl = QString(
            "https://api.technicpack.net/search?build=multimc&q=%1"
        ).arg(currentSearchTerm);
    }
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start(APPLICATION->network());
    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void Technic::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError)
    {
        qWarning() << "Error while parsing JSON response from Technic at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QList<Modpack> newList;
    try {
        auto root = Json::requireObject(doc);
        auto objs = Json::requireArray(root, "modpacks");
        for (auto technicPack: objs) {
            Modpack pack;
            auto technicPackObject = Json::requireObject(technicPack);
            pack.name = Json::requireString(technicPackObject, "name");
            pack.slug = Json::requireString(technicPackObject, "slug");
            if (pack.slug == "vanilla")
                continue;

            auto rawURL = Json::ensureString(technicPackObject, "iconUrl", "null");
            if(rawURL == "null") {
                pack.logoUrl = "null";
                pack.logoName = "null";
            }
            else {
                pack.logoUrl = rawURL;
                pack.logoName = rawURL.section(QLatin1Char('/'), -1).section(QLatin1Char('.'), 0, 0);
            }
            pack.broken = false;
            newList.append(pack);
        }
    }
    catch (const JSONValidationError &err)
    {
        qCritical() << "Couldn't parse technic search results:" << err.cause() ;
        return;
    }
    searchState = Finished;
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void Technic::ListModel::getLogo(const QString& logo, const QString& logoUrl, Technic::LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(APPLICATION->metacache()->resolveEntry("TechnicPacks", QString("logos/%1").arg(logo))->getFullPath());
    }
    else
    {
        requestLogo(logo, logoUrl);
    }
}

void Technic::ListModel::searchRequestFailed()
{
    jobPtr.reset();

    if(searchState == ResetRequested)
    {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        performSearch();
    }
    else
    {
        searchState = Finished;
    }
}


void Technic::ListModel::logoLoaded(QString logo, QString out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, QIcon(out));
    for(int i = 0; i < modpacks.size(); i++)
    {
        if(modpacks[i].logoName == logo)
        {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
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
    if(m_loadingLogos.contains(logo) || m_failedLogos.contains(logo) || logo == "null")
    {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("TechnicPacks", QString("logos/%1").arg(logo));
    NetJob *job = new NetJob(QString("Technic Icon Download %1").arg(logo));
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();

    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath]
    {
        logoLoaded(logo, fullPath);
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo]
    {
        logoFailed(logo);
    });

    job->start(APPLICATION->network());

    m_loadingLogos.append(logo);
}
