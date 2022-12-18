#include "ModModel.h"

#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include <QMessageBox>

namespace ResourceDownload {

ModModel::ModModel(BaseInstance const& base_inst, ResourceAPI* api) : ResourceModel(base_inst, api) {}

/******** Make data requests ********/

ResourceAPI::SearchArgs ModModel::createSearchArguments()
{
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions {};
    if (!m_filter->versions.empty())
        versions = m_filter->versions;

    return { ModPlatform::ResourceType::MOD, m_next_search_offset,     m_search_term,
             getSorts()[currentSort],        profile->getModLoaders(), versions };
}
ResourceAPI::SearchCallbacks ModModel::createSearchCallbacks()
{
    return { [this](auto& doc) {
        if (!s_running_models.constFind(this).value())
            return;
        searchRequestFinished(doc);
    } };
}

ResourceAPI::VersionSearchArgs ModModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions {};
    if (!m_filter->versions.empty()) 
        versions = m_filter->versions;

    return { pack, versions, profile->getModLoaders() };
}
ResourceAPI::VersionSearchCallbacks ModModel::createVersionsCallbacks(QModelIndex& entry)
{
    return { [this, entry](auto& doc, auto& pack) {
        if (!s_running_models.constFind(this).value())
            return;
        versionRequestSucceeded(doc, pack, entry);
    } };
}

ResourceAPI::ProjectInfoArgs ModModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { pack };
}
ResourceAPI::ProjectInfoCallbacks ModModel::createInfoCallbacks(QModelIndex& entry)
{
    return { [this, entry](auto& doc, auto& pack) {
        if (!s_running_models.constFind(this).value())
            return;
        infoRequestFinished(doc, pack, entry);
    } };
}

void ModModel::searchWithTerm(const QString& term, const int sort, const bool filter_changed)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && currentSort == sort && !filter_changed) {
        return;
    }

    setSearchTerm(term);
    currentSort = sort;

    refresh();
}

/******** Request callbacks ********/

void ModModel::searchRequestFinished(QJsonDocument& doc)
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
            qWarning() << "Error while loading mod from " << debugName() << ": " << e.cause();
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

void ModModel::infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack, const QModelIndex& index)
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
            return;
        }
        
        emit projectInfoUpdated();
    }
}

void ModModel::versionRequestSucceeded(QJsonDocument doc, ModPlatform::IndexedPack& pack, const QModelIndex& index)
{
    auto arr = doc.isObject() ? Json::ensureArray(doc.object(), "data") : doc.array();

    try {
        loadIndexedPackVersions(pack, arr);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << debugName() << " mod version: " << e.cause();
    }

    // Check if the index is still valid for this mod or not
    if (pack.addonId == data(index, Qt::UserRole).value<ModPlatform::IndexedPack>().addonId) {
        // Cache info :^)
        QVariant new_pack;
        new_pack.setValue(pack);
        if (!setData(index, new_pack, Qt::UserRole)) {
            qWarning() << "Failed to cache mod versions!";
            return;
        }

        emit versionListUpdated();
    }
}

}  // namespace ResourceDownload
