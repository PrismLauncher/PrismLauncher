#include "ModrinthModel.h"

#include "BuildConfig.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

#include <QMessageBox>

namespace Modrinth {

ModpackListModel::ModpackListModel(ModrinthPage* parent) : QAbstractListModel(parent), m_parent(parent) {}

auto ModpackListModel::debugName() const -> QString
{
    return m_parent->debugName();
}

/******** Make data requests ********/

void ModpackListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if (nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

auto ModpackListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    Modrinth::Modpack pack = modpacks.at(pos);
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
        if (m_logoMap.contains(pack.iconName)) {
            return (m_logoMap.value(pack.iconName)
                        .pixmap(48, 48)
                        .scaled(48, 48, Qt::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation));
        }
        QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
        ((ModpackListModel*)this)->requestLogo(pack.iconName, pack.iconUrl.toString());
        return icon;
    } else if (role == Qt::UserRole) {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return {};
}

void ModpackListModel::performPaginatedSearch()
{
    // TODO: Move to standalone API
    NetJob* netJob = new NetJob("Modrinth::SearchModpack", APPLICATION->network());
    auto searchAllUrl = QString(
                            "%1/search?"
                            "query=%2&"
                            "facets=[[\"project_type:modpack\"]]")
                            .arg(BuildConfig.MODRINTH_STAGING_URL, currentSearchTerm);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchAllUrl), &m_all_response));

    QObject::connect(netJob, &NetJob::succeeded, this, [this] {
        QJsonParseError parse_error_all{};

        QJsonDocument doc_all = QJsonDocument::fromJson(m_all_response, &parse_error_all);
        if (parse_error_all.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << debugName() << " at " << parse_error_all.offset
                       << " reason: " << parse_error_all.errorString();
            qWarning() << m_all_response;
            return;
        }

        searchRequestFinished(doc_all);
    });
    QObject::connect(netJob, &NetJob::failed, this, &ModpackListModel::searchRequestFailed);

    jobPtr = netJob;
    jobPtr->start();
}

void ModpackListModel::refresh()
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

void ModpackListModel::searchWithTerm(const QString& term, const int sort)
{
    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort) {
        return;
    }

    currentSearchTerm = term;
    currentSort = sort;

    refresh();
}

void ModpackListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()
                     ->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo.section(".", 0, 0)))
                     ->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void ModpackListModel::requestLogo(QString logo, QString url)
{
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo)) {
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

void ModpackListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].iconName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
        }
    }
}

void ModpackListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ModpackListModel::searchRequestFinished(QJsonDocument& doc_all)
{
    jobPtr.reset();

    QList<Modrinth::Modpack> newList;

    auto packs_all = doc_all.object().value("hits").toArray();
    for (auto packRaw : packs_all) {
        auto packObj = packRaw.toObject();

        Modrinth::Modpack pack;
        try {
            Modrinth::loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading mod from " << m_parent->debugName() << ": " << e.cause();
            continue;
        }
    }

    if (packs_all.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void ModpackListModel::searchRequestFailed(QString reason)
{
    if (!jobPtr->first()->m_reply) {
        // Network error
        QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load mods."));
    } else if (jobPtr->first()->m_reply && jobPtr->first()->m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409) {
        // 409 Gone, notify user to update
        QMessageBox::critical(nullptr, tr("Error"),
                              //: %1 refers to the launcher itself
                              QString("%1 %2")
                                  .arg(m_parent->displayName())
                                  .arg(tr("API version too old!\nPlease update %1!").arg(BuildConfig.LAUNCHER_NAME)));
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

void ModpackListModel::versionRequestSucceeded(QJsonDocument doc, QString id)
{
    auto& current = m_parent->getCurrent();
    if (id != current.id) {
        return;
    }

    auto arr = doc.isObject() ? Json::ensureArray(doc.object(), "data") : doc.array();

    try {
        // loadIndexedPackVersions(current, arr);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << debugName() << " mod version: " << e.cause();
    }

    // m_parent->updateModVersions();
}

}  // namespace Modrinth

/******** Helpers ********/
