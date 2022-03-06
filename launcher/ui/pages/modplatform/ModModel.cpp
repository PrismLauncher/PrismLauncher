#include "ModModel.h"
#include "ModPage.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

#include <QMessageBox>

namespace ModPlatform {

ListModel::ListModel(ModPage* parent) : QAbstractListModel(parent), m_parent(parent) {}

ListModel::~ListModel() {}

int ListModel::rowCount(const QModelIndex& parent) const
{
    return modpacks.size();
}

int ListModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) { return QString("INVALID INDEX %1").arg(pos); }

    ModPlatform::IndexedPack pack = modpacks.at(pos);
    if (role == Qt::DisplayRole) {
        return pack.name;
    } else if (role == Qt::ToolTipRole) {
        if (pack.description.length() > 100) {
            // some magic to prevent to long tooltips and replace html linebreaks
            QString edit = pack.description.left(97);
            edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
            return edit;
        }
        return pack.description;
    } else if (role == Qt::DecorationRole) {
        if (m_logoMap.contains(pack.logoName)) { return (m_logoMap.value(pack.logoName)); }
        QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
        ((ListModel*)this)->requestLogo(pack.logoName, pack.logoUrl);
        return icon;
    } else if (role == Qt::UserRole) {
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
    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].logoName == logo) { emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole }); }
    }
}

void ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

Qt::ItemFlags ListModel::flags(const QModelIndex& index) const
{
    return QAbstractListModel::flags(index);
}

bool ListModel::canFetchMore(const QModelIndex& parent) const
{
    return searchState == CanPossiblyFetchMore;
}

void ListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid()) return;
    if (nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

void ListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::populateVersions(ModPlatform::IndexedPack const& current)
{
    auto netJob = new NetJob(QString("%1::ModVersions(%2)").arg(m_parent->debugName()).arg(current.name), APPLICATION->network());
    auto response = new QByteArray();
    QString addonId = current.addonId.toString();

    netJob->addNetAction(Net::Download::makeByteArray(m_parent->apiProvider()->getVersionsURL(addonId), response));

    QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId]{
        m_parent->onGetVersionsSucceeded(m_parent, response, addonId);
    });

    QObject::connect(netJob, &NetJob::finished, this, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });

    netJob->start();
}

void ListModel::performPaginatedSearch()
{
    QString mcVersion = ((MinecraftInstance*)((ModPage*)parent())->m_instance)->getPackProfile()->getComponentVersion("net.minecraft");
    bool hasFabric = !((MinecraftInstance*)((ModPage*)parent())->m_instance)
                          ->getPackProfile()
                          ->getComponentVersion("net.fabricmc.fabric-loader")
                          .isEmpty();
    auto netJob = new NetJob(QString("%1::Search").arg(m_parent->debugName()), APPLICATION->network());
    auto searchUrl = m_parent->apiProvider()->getModSearchURL(nextSearchOffset, currentSearchTerm, getSorts()[currentSort], hasFabric, mcVersion);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::searchWithTerm(const QString& term, const int sort)
{
    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort) { return; }
    currentSearchTerm = term;
    currentSort = sort;
    if (jobPtr) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    } else {
        beginResetModel();
        modpacks.clear();
        endResetModel();
        searchState = None;
    }
    nextSearchOffset = 0;
    performPaginatedSearch();
}

void ListModel::searchRequestFailed(QString reason)
{
    if (jobPtr->first()->m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409) {
        // 409 Gone, notify user to update
        QMessageBox::critical(nullptr, tr("Error"),
                              QString("%1 %2")
                                .arg(m_parent->displayName())
                                .arg(tr("API version too old!\nPlease update PolyMC!")));
        // self-destruct
        ((ModDownloadDialog*)((ModPage*)parent())->parentWidget())->reject();
    }
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

void ListModel::requestLogo(QString logo, QString url)
{
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo)) { return; }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo.section(".", 0, 0)));
    auto job = new NetJob(QString("%1 Icon Download %2").arg(m_parent->debugName()).arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(logo, QIcon(fullPath));
        if (waitingCallbacks.contains(logo)) { waitingCallbacks.value(logo)(fullPath); }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo, job] {
        job->deleteLater();
        emit logoFailed(logo);
    });

    job->start();
    m_loadingLogos.append(logo);
}

}  // namespace ModPlatform
