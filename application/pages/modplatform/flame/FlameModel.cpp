#include "FlameModel.h"
#include "MultiMC.h"

#include <MMCStrings.h>
#include <Version.h>

#include <QtMath>
#include <QLabel>

#include <RWStorage.h>
#include <Env.h>

namespace Flame {

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

    Modpack pack = modpacks.at(pos);
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
        QIcon icon = MMC->getThemedIcon("screenshot-placeholder");
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

    MetaEntryPtr entry = ENV.metacache()->resolveEntry("FlamePacks", QString("logos/%1").arg(logo.section(".", 0, 0)));
    NetJob *job = new NetJob(QString("Flame Icon Download %1").arg(logo));
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
        callback(ENV.metacache()->resolveEntry("FlamePacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
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

void ListModel::performPaginatedSearch()
{
    NetJob *netJob = new NetJob("Flame::Search");
    auto searchUrl = QString(
        "https://addons-ecs.forgesvc.net/api/v2/addon/search?"
        "categoryId=0&"
        "gameId=432&"
        //"gameVersion=1.12.2&"
        "index=%1&"
        "pageSize=25&"
        "searchFilter=%2&"
        "sectionId=4471&"
        "sort=0"
    ).arg(nextSearchOffset).arg(currentSearchTerm);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::searchWithTerm(const QString& term)
{
    if(currentSearchTerm == term) {
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
    nextSearchOffset = 0;
    performPaginatedSearch();
}

void Flame::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QList<Modpack> newList;
    auto objs = doc.array();
    for(auto projectIter: objs) {
        Modpack pack;
        auto project = projectIter.toObject();
        pack.addonId = project.value("id").toInt(0);
        if (pack.addonId == 0) {
            qWarning() << "Pack without an ID, skipping: " << pack.name;
            continue;
        }
        pack.name = project.value("name").toString();
        pack.websiteUrl = project.value("websiteUrl").toString();
        pack.description = project.value("summary").toString();
        bool thumbnailFound = false;
        auto attachments = project.value("attachments").toArray();
        for(auto attachmentIter: attachments) {
            auto attachment = attachmentIter.toObject();
            bool isDefault = attachment.value("isDefault").toBool(false);
            if(isDefault) {
                thumbnailFound = true;
                pack.logoName = attachment.value("title").toString();
                pack.logoUrl = attachment.value("thumbnailUrl").toString();
                break;
            }
        }
        if(!thumbnailFound) {
            qWarning() << "Pack without an icon, skipping: " << pack.name;
            continue;
        }
        auto authors = project.value("authors").toArray();
        for(auto authorIter: authors) {
            auto author = authorIter.toObject();
            ModpackAuthor packAuthor;
            packAuthor.name = author.value("name").toString();
            packAuthor.url = author.value("url").toString();
            pack.authors.append(packAuthor);
        }
        int defaultFileId = project.value("defaultFileId").toInt(0);
        if(defaultFileId == 0) {
            qWarning() << "Pack without default file, skipping: " << pack.name;
            continue;
        }
        bool found = false;
        auto files = project.value("latestFiles").toArray();
        for(auto fileIter: files) {
            auto file = fileIter.toObject();
            int id = file.value("id").toInt(0);
            // NOTE: for now, ignore everything that's not the default...
            if(id != defaultFileId) {
                continue;
            }
            pack.latestFile.addonId = pack.addonId;
            pack.latestFile.fileId = id;
            auto versionArray = file.value("gameVersion").toArray();
            if(versionArray.size() < 1) {
                continue;
            }

            // pick the latest version supported
            pack.latestFile.mcVersion = versionArray[0].toString();
            pack.latestFile.version = file.value("displayName").toString();
            pack.latestFile.downloadUrl = file.value("downloadUrl").toString();
            found = true;
            break;
        }
        if(!found) {
            qWarning() << "Pack with no good file, skipping: " << pack.name;
            continue;
        }
        pack.broken = false;
        newList.append(pack);
    }
    if(objs.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void Flame::ListModel::searchRequestFailed(QString reason)
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

