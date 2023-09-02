#include "FlameModel.h"
#include <Json.h>
#include "Application.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameAPI.h"
#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

#include <Version.h>

#include <QtMath>

namespace Flame {

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

    IndexedPack pack = modpacks.at(pos);
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
        case Qt::SizeHintRole:
            return QSize(0, 58);
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
    return QVariant();
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, [[maybe_unused]] int role)
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid())
        return false;

    modpacks[pos] = value.value<Flame::IndexedPack>();

    return true;
}

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].logoName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
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
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo)) {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("FlamePacks", QString("logos/%1").arg(logo));
    auto job = new NetJob(QString("Flame Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::ApiDownload::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(logo, QIcon(fullPath));
        if (waitingCallbacks.contains(logo)) {
            waitingCallbacks.value(logo)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo, job] {
        job->deleteLater();
        emit logoFailed(logo);
    });

    job->start();

    m_loadingLogos.append(logo);
}

void ListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()->resolveEntry("FlamePacks", QString("logos/%1").arg(logo))->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

Qt::ItemFlags ListModel::flags(const QModelIndex& index) const
{
    return QAbstractListModel::flags(index);
}

bool ListModel::canFetchMore([[maybe_unused]] const QModelIndex& parent) const
{
    return searchState == CanPossiblyFetchMore;
}

void ListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if (nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

void ListModel::performPaginatedSearch()
{
    if (currentSearchTerm.startsWith("#")) {
        auto projectId = currentSearchTerm.mid(1);
        if (!projectId.isEmpty()) {
            ResourceAPI::ProjectInfoCallbacks callbacks;

            callbacks.on_fail = [this](QString reason) { searchRequestFailed(reason); };
            callbacks.on_succeed = [this](auto& doc, auto& pack) { searchRequestForOneSucceeded(doc); };
            static const FlameAPI api;
            if (auto job = api.getProjectInfo({ projectId }, std::move(callbacks)); job) {
                jobPtr = job;
                jobPtr->start();
            }
            return;
        }
    }
    auto netJob = makeShared<NetJob>("Flame::Search", APPLICATION->network());
    auto searchUrl = QString(
                         "https://api.curseforge.com/v1/mods/search?"
                         "gameId=432&"
                         "classId=4471&"
                         "index=%1&"
                         "pageSize=25&"
                         "searchFilter=%2&"
                         "sortField=%3&"
                         "sortOrder=desc")
                         .arg(nextSearchOffset)
                         .arg(currentSearchTerm)
                         .arg(currentSort + 1);

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(searchUrl), response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob.get(), &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob.get(), &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::searchWithTerm(const QString& term, int sort)
{
    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort) {
        return;
    }
    currentSearchTerm = term;
    currentSort = sort;
    if (hasActiveSearchJob()) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    }
    beginResetModel();
    modpacks.clear();
    endResetModel();
    searchState = None;

    nextSearchOffset = 0;
    performPaginatedSearch();
}

void Flame::ListModel::searchRequestFinished()
{
    if (hasActiveSearchJob())
        return;

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
    }

    QList<Flame::IndexedPack> newList;
    auto packs = Json::ensureArray(doc.object(), "data");
    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        Flame::IndexedPack pack;
        try {
            Flame::loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading pack from CurseForge: " << e.cause();
            continue;
        }
    }
    if (packs.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void Flame::ListModel::searchRequestForOneSucceeded(QJsonDocument& doc)
{
    jobPtr.reset();

    auto packObj = Json::ensureObject(doc.object(), "data");

    Flame::IndexedPack pack;
    try {
        Flame::loadIndexedPack(pack, packObj);
    } catch (const JSONValidationError& e) {
        qWarning() << "Error while loading pack from CurseForge: " << e.cause();
        return;
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + 1);
    modpacks.append({ pack });
    endInsertRows();
}

void Flame::ListModel::searchRequestFailed(QString reason)
{
    jobPtr.reset();

    if (searchState == ResetRequested) {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        nextSearchOffset = 0;
        performPaginatedSearch();
    } else {
        searchState = Finished;
    }
}

}  // namespace Flame
