#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include "ui/pages/modplatform/ResourceModel.h"

class ModPage;
class Version;

namespace ModPlatform {

class ListModel : public ResourceModel {
    Q_OBJECT

   public:
    ListModel(ModPage* parent, ResourceAPI* api);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, const int sort, const bool filter_changed);

    virtual void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) = 0;

   public slots:
    void searchRequestFinished(QJsonDocument& doc);

    void infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack, const QModelIndex& index);

    void versionRequestSucceeded(QJsonDocument doc, QString addonId, const QModelIndex& index);

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::SearchCallbacks createSearchCallbacks() override;

    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;
    ResourceAPI::VersionSearchCallbacks createVersionsCallbacks(QModelIndex&) override;

    ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) override;
    ResourceAPI::ProjectInfoCallbacks createInfoCallbacks(QModelIndex&) override;

   protected:
    virtual auto documentToArray(QJsonDocument& obj) const -> QJsonArray = 0;
    virtual auto getSorts() const -> const char** = 0;

    inline auto getMineVersions() const -> std::optional<std::list<Version>>;

   protected:
    int currentSort = 0;
};
}  // namespace ModPlatform
