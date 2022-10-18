#include "ModModel.h"

#include "BuildConfig.h"
#include "Json.h"
#include "ModPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

#include "ui/widgets/ProjectItem.h"

#include <QMessageBox>

namespace ModPlatform {

// HACK: We need this to prevent callbacks from calling the ListModel after it has already been deleted.
// This leaks a tiny bit of memory per time the user has opened the mod dialog. How to make this better?
static QHash<ListModel*, bool> s_running;

ListModel::ListModel(ModPage* parent) : QAbstractListModel(parent), m_parent(parent) { s_running.insert(this, true); }

ListModel::~ListModel()
{
    s_running.find(this).value() = false;
}

auto ListModel::debugName() const -> QString
{
    return m_parent->debugName();
}

/******** Make data requests ********/

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

auto ListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    ModPlatform::IndexedPack pack = modpacks.at(pos);
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
                return m_logoMap.value(pack.logoName);
            }
            QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
            // un-const-ify this
            ((ListModel*)this)->requestLogo(pack.logoName, pack.logoUrl);
            return icon;
        }
        case Qt::SizeHintRole: 
            return QSize(0, 58);
        case Qt::UserRole: {
            QVariant v;
            v.setValue(pack);
            return v;
        }
    // Custom data
        case UserDataTypes::TITLE:
            return pack.name;
        case UserDataTypes::DESCRIPTION:
            return pack.description;
        case UserDataTypes::SELECTED:
            return m_parent->getDialog()->isModSelected(pack.name);
        default:
            break;
    }

    return {};
}

bool ListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid())
        return false;

    modpacks[pos] = value.value<ModPlatform::IndexedPack>();

    return true;
}

void ListModel::requestModVersions(ModPlatform::IndexedPack const& current, QModelIndex index)
{
    auto profile = (dynamic_cast<MinecraftInstance*>((dynamic_cast<ModPage*>(parent()))->m_instance))->getPackProfile();

    m_parent->apiProvider()->getVersions({ current.addonId.toString(), getMineVersions(), profile->getModLoaders() },
                                         [this, current, index](QJsonDocument& doc, QString addonId) {
                                             if (!s_running.constFind(this).value())
                                                 return;
                                             versionRequestSucceeded(doc, addonId, index);
                                         });
}

void ListModel::performPaginatedSearch()
{
    auto profile = (dynamic_cast<MinecraftInstance*>((dynamic_cast<ModPage*>(parent()))->m_instance))->getPackProfile();

    m_parent->apiProvider()->searchMods(
        this, { nextSearchOffset, currentSearchTerm, getSorts()[currentSort], profile->getModLoaders(), getMineVersions() });
}

void ListModel::requestModInfo(ModPlatform::IndexedPack& current, QModelIndex index)
{
    m_parent->apiProvider()->getModInfo(current, [this, index](QJsonDocument& doc, ModPlatform::IndexedPack& pack) {
        if (!s_running.constFind(this).value())
            return;
        infoRequestFinished(doc, pack, index);
    });
}

void ListModel::refresh()
{
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

void ListModel::searchWithTerm(const QString& term, const int sort, const bool filter_changed)
{
    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort && !filter_changed) {
        return;
    }

    currentSearchTerm = term;
    currentSort = sort;

    refresh();
}

void ListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()
                     ->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo.section(".", 0, 0)))
                     ->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::requestLogo(QString logo, QString url)
{
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo) || url.isEmpty()) {
        return;
    }

    MetaEntryPtr entry =
        APPLICATION->metacache()->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo.section(".", 0, 0)));
    auto job = new NetJob(QString("%1 Icon Download %2").arg(m_parent->debugName()).arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

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

/******** Request callbacks ********/

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

void ListModel::searchRequestFinished(QJsonDocument& doc)
{
    jobPtr.reset();

    QList<ModPlatform::IndexedPack> newList;
    auto packs = documentToArray(doc);

    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        try {
            loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading mod from " << m_parent->debugName() << ": " << e.cause();
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

void ListModel::searchRequestFailed(QString reason)
{
    auto failed_action = jobPtr->getFailedActions().at(0);
    if (!failed_action->m_reply) {
        // Network error
        QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load mods."));
    } else if (failed_action->m_reply && failed_action->m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409) {
        // 409 Gone, notify user to update
        QMessageBox::critical(nullptr, tr("Error"),
                              //: %1 refers to the launcher itself
                              QString("%1 %2")
                                  .arg(m_parent->displayName())
                                  .arg(tr("API version too old!\nPlease update %1!").arg(BuildConfig.LAUNCHER_DISPLAYNAME)));
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

void ListModel::infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack, const QModelIndex& index)
{
    qDebug() << "Loading mod info";

    try {
        auto obj = Json::requireObject(doc);
        loadExtraPackInfo(pack, obj);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << debugName() << " mod info: " << e.cause();
    }

    // Check if the index is still valid for this mod or not
    if (pack.addonId == data(index, Qt::UserRole).value<ModPlatform::IndexedPack>().addonId) {
        // Cache info :^)
        QVariant new_pack;
        new_pack.setValue(pack);
        if (!setData(index, new_pack, Qt::UserRole)) {
            qWarning() << "Failed to cache mod info!";
        }
    }

    m_parent->updateUi();
}

void ListModel::versionRequestSucceeded(QJsonDocument doc, QString addonId, const QModelIndex& index)
{
    auto& current = m_parent->getCurrent();
    if (addonId != current.addonId) {
        return;
    }

    auto arr = doc.isObject() ? Json::ensureArray(doc.object(), "data") : doc.array();

    try {
        loadIndexedPackVersions(current, arr);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << debugName() << " mod version: " << e.cause();
    }

    // Cache info :^)
    QVariant new_pack;
    new_pack.setValue(current);
    if (!setData(index, new_pack, Qt::UserRole)) {
        qWarning() << "Failed to cache mod versions!";
    }


    m_parent->updateModVersions();
}

}  // namespace ModPlatform

/******** Helpers ********/

auto ModPlatform::ListModel::getMineVersions() const -> std::list<Version>
{
    return m_parent->getFilter()->versions;
}
