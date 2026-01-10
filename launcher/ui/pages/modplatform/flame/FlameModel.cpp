#include "FlameModel.h"
#include <Json.h>
#include "Application.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/flame/FlameAPI.h"
#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

#include <Version.h>

#include <QtMath>
#include <memory>

namespace Flame {

ListModel::ListModel(QObject* parent) : QAbstractListModel(parent) {}

ListModel::~ListModel() {}

int ListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_modpacks.size();
}

int ListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if (pos >= m_modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    auto pack = m_modpacks.at(pos);
    switch (role) {
        case Qt::ToolTipRole: {
            if (pack->description.length() > 100) {
                // some magic to prevent to long tooltips and replace html linebreaks
                QString edit = pack->description.left(97);
                edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
                return edit;
            }
            return pack->description;
        }
        case Qt::DecorationRole: {
            if (m_logoMap.contains(pack->logoName)) {
                return (m_logoMap.value(pack->logoName));
            }
            QIcon icon = QIcon::fromTheme("screenshot-placeholder");
            ((ListModel*)this)->requestLogo(pack->logoName, pack->logoUrl);
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
            return pack->name;
        case UserDataTypes::DESCRIPTION:
            return pack->description;
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
    if (pos >= m_modpacks.size() || pos < 0 || !index.isValid())
        return false;

    m_modpacks[pos] = value.value<ModPlatform::IndexedPack::Ptr>();

    return true;
}

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for (int i = 0; i < m_modpacks.size(); i++) {
        if (m_modpacks[i]->logoName == logo) {
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
    job->setAskRetry(false);
    job->addNetAction(Net::ApiDownload::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(logo, QIcon(fullPath));
        if (m_waitingCallbacks.contains(logo)) {
            m_waitingCallbacks.value(logo)(fullPath);
        }
    });

    connect(job, &NetJob::failed, this, [this, logo, job] {
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
    return m_searchState == CanPossiblyFetchMore;
}

void ListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if (m_nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

void ListModel::performPaginatedSearch()
{
    static const FlameAPI api;
    if (m_currentSearchTerm.startsWith("#")) {
        auto projectId = m_currentSearchTerm.mid(1);
        if (!projectId.isEmpty()) {
            ResourceAPI::Callback<ModPlatform::IndexedPack::Ptr> callbacks;

            callbacks.on_fail = [this](QString reason, int) { searchRequestFailed(reason); };
            callbacks.on_succeed = [this](auto& pack) { searchRequestForOneSucceeded(pack); };
            callbacks.on_abort = [this] {
                qCritical() << "Search task aborted by an unknown reason!";
                searchRequestFailed("Aborted");
            };
            auto project = std::make_shared<ModPlatform::IndexedPack>();
            project->addonId = projectId;
            if (auto job = api.getProjectInfo({ project }, std::move(callbacks)); job) {
                m_jobPtr = job;
                m_jobPtr->start();
            }
            return;
        }
    }
    ResourceAPI::SortingMethod sort{};
    sort.index = m_currentSort + 1;

    ResourceAPI::Callback<QList<ModPlatform::IndexedPack::Ptr>> callbacks{};

    callbacks.on_succeed = [this](auto& doc) { searchRequestFinished(doc); };
    callbacks.on_fail = [this](QString reason, int) { searchRequestFailed(reason); };
    callbacks.on_abort = [this] {
        qCritical() << "Search task aborted by an unknown reason!";
        searchRequestFailed("Aborted");
    };

    auto netJob = api.searchProjects({ ModPlatform::ResourceType::Modpack, m_nextSearchOffset, m_currentSearchTerm, sort, m_filter->loaders,
                                       m_filter->versions, ModPlatform::Side::NoSide, m_filter->categoryIds, m_filter->openSource },
                                     std::move(callbacks));

    m_jobPtr = netJob;
    m_jobPtr->start();
}

void ListModel::searchWithTerm(const QString& term, int sort, std::shared_ptr<ModFilterWidget::Filter> filter, bool filterChanged)
{
    if (m_currentSearchTerm == term && m_currentSearchTerm.isNull() == term.isNull() && m_currentSort == sort && !filterChanged) {
        return;
    }
    m_currentSearchTerm = term;
    m_currentSort = sort;
    m_filter = filter;
    if (hasActiveSearchJob()) {
        m_jobPtr->abort();
        m_searchState = ResetRequested;
        return;
    }
    beginResetModel();
    m_modpacks.clear();
    endResetModel();
    m_searchState = None;

    m_nextSearchOffset = 0;
    performPaginatedSearch();
}

void Flame::ListModel::searchRequestFinished(QList<ModPlatform::IndexedPack::Ptr>& newList)
{
    if (hasActiveSearchJob())
        return;

    if (newList.size() < 25) {
        m_searchState = Finished;
    } else {
        m_nextSearchOffset += 25;
        m_searchState = CanPossiblyFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), m_modpacks.size(), m_modpacks.size() + newList.size() - 1);
    m_modpacks.append(newList);
    endInsertRows();
}

void Flame::ListModel::searchRequestForOneSucceeded(ModPlatform::IndexedPack::Ptr pack)
{
    m_jobPtr.reset();

    beginInsertRows(QModelIndex(), m_modpacks.size(), m_modpacks.size() + 1);
    m_modpacks.append(pack);
    endInsertRows();
}

void Flame::ListModel::searchRequestFailed(QString reason)
{
    m_jobPtr.reset();

    if (m_searchState == ResetRequested) {
        beginResetModel();
        m_modpacks.clear();
        endResetModel();

        m_nextSearchOffset = 0;
        performPaginatedSearch();
    } else {
        m_searchState = Finished;
    }
}

}  // namespace Flame
