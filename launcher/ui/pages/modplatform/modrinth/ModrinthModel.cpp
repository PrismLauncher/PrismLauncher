#include "ModrinthModel.h"
#include "Application.h"
#include <Json.h>

#include <MMCStrings.h>
#include <Version.h>

#include <QtMath>
#include <QLabel>

#include <RWStorage.h>

namespace Modrinth {

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

    IndexedPack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if (role == Qt::ToolTipRole)
    {
        if(pack.description.length() > 100)
        {
            //some magic to prevent to long tooltips and replace html linebreaks
            QString edit = pack.description.left(97);
            edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
            return edit;

        }
        return pack.description;
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

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].logoName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
        }
    }
}

void ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ListModel::requestLogo(QString logo, QString url)
{
    if(m_loadingLogos.contains(logo) || m_failedLogos.contains(logo))
    {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("ModrinthPacks", QString("logos/%1").arg(logo.section(".", 0, 0)));
    NetJob *job = new NetJob(QString("Modrinth Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath]
    {
        emit logoLoaded(logo, QIcon(fullPath));
        if(waitingCallbacks.contains(logo))
        {
            waitingCallbacks.value(logo)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo]
    {
        emit logoFailed(logo);
    });

    job->start();

    m_loadingLogos.append(logo);
}

void ListModel::getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(APPLICATION->metacache()->resolveEntry("ModrinthPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    }
    else
    {
        requestLogo(logo, logoUrl);
    }
}

Qt::ItemFlags ListModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index);
}

bool ListModel::canFetchMore(const QModelIndex& parent) const
{
    return searchState == CanPossiblyFetchMore;
}

void ListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if(nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}
const char* sorts[4]{"relevance","downloads","updated","newest"};

void ListModel::performPaginatedSearch()
{
    NetJob *netJob = new NetJob("Modrinth::Search", APPLICATION->network());
    auto searchUrl = QString(
        "https://api.modrinth.com/api/v1/mod?"
        "offset=%1&"
        "limit=25&"
        "query=%2&"
        "index=%3"
    ).arg(nextSearchOffset).arg(currentSearchTerm).arg(sorts[currentSort]);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::searchWithTerm(const QString& term, int sort)
{
    if(currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort) {
        return;
    }
    currentSearchTerm = term;
    currentSort = sort;
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
    nextSearchOffset = 0;
    performPaginatedSearch();
}

void Modrinth::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QList<Modrinth::IndexedPack> newList;
    auto packs = doc.object().value("hits").toArray();
    for(auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        Modrinth::IndexedPack pack;
        try
        {
            Modrinth::loadIndexedPack(pack, packObj);
            newList.append(pack);
        }
        catch(const JSONValidationError &e)
        {
            qWarning() << "Error while loading mod from Modrinth: " << e.cause();
            continue;
        }
    }
    if(packs.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void Modrinth::ListModel::searchRequestFailed(QString reason)
{
    jobPtr.reset();

    if(searchState == ResetRequested) {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        nextSearchOffset = 0;
        performPaginatedSearch();
    } else {
        searchState = Finished;
    }
}

}

