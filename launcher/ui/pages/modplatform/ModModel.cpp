#include "ModModel.h"

#include "Json.h"
#include "ModPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include <QMessageBox>

namespace ModPlatform {

ListModel::ListModel(ModPage* parent, ResourceAPI* api) : ResourceModel(parent, api) {}

/******** Make data requests ********/

ResourceAPI::SearchArgs ListModel::createSearchArguments()
{
    auto profile = static_cast<MinecraftInstance&>(m_associated_page->m_base_instance).getPackProfile();
    return { ModPlatform::ResourceType::MOD, m_next_search_offset,     m_search_term,
             getSorts()[currentSort],        profile->getModLoaders(), getMineVersions() };
}
ResourceAPI::SearchCallbacks ListModel::createSearchCallbacks()
{
    return { [this](auto& doc) {
        if (!s_running_models.constFind(this).value())
            return;
        searchRequestFinished(doc);
    } };
}

ResourceAPI::VersionSearchArgs ListModel::createVersionsArguments(QModelIndex& entry)
{
    auto const& pack = m_packs[entry.row()];
    auto profile = static_cast<MinecraftInstance&>(m_associated_page->m_base_instance).getPackProfile();

    return { pack.addonId.toString(), getMineVersions(), profile->getModLoaders() };
}
ResourceAPI::VersionSearchCallbacks ListModel::createVersionsCallbacks(QModelIndex& entry)
{
    auto const& pack = m_packs[entry.row()];

    return { [this, pack, entry](auto& doc, auto addonId) {
        if (!s_running_models.constFind(this).value())
            return;
        versionRequestSucceeded(doc, addonId, entry);
    } };
}

ResourceAPI::ProjectInfoArgs ListModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { pack };
}
ResourceAPI::ProjectInfoCallbacks ListModel::createInfoCallbacks(QModelIndex& entry)
{
    return { [this, entry](auto& doc, auto& pack) {
        if (!s_running_models.constFind(this).value())
            return;
        infoRequestFinished(doc, pack, entry);
    } };
}

void ListModel::searchWithTerm(const QString& term, const int sort, const bool filter_changed)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && currentSort == sort && !filter_changed) {
        return;
    }

    setSearchTerm(term);
    currentSort = sort;

    refresh();
}

/******** Request callbacks ********/

void ListModel::searchRequestFinished(QJsonDocument& doc)
{
    QList<ModPlatform::IndexedPack> newList;
    auto packs = documentToArray(doc);

    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        try {
            loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading mod from " << m_associated_page->debugName() << ": " << e.cause();
            continue;
        }
    }

    if (packs.size() < 25) {
        m_search_state = SearchState::Finished;
    } else {
        m_next_search_offset += 25;
        m_search_state = SearchState::CanFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), m_packs.size(), m_packs.size() + newList.size() - 1);
    m_packs.append(newList);
    endInsertRows();
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

    m_associated_page->updateUi();
}

void ListModel::versionRequestSucceeded(QJsonDocument doc, QString addonId, const QModelIndex& index)
{
    auto current = m_associated_page->getCurrentPack();
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

    m_associated_page->updateVersionList();
}

}  // namespace ModPlatform

/******** Helpers ********/

#define MOD_PAGE(x) static_cast<ModPage*>(x)

auto ModPlatform::ListModel::getMineVersions() const -> std::optional<std::list<Version>>
{
    auto versions = MOD_PAGE(m_associated_page)->getFilter()->versions;
    if (!versions.empty())
        return versions;
    return {};
}
