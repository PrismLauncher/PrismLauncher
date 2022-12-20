#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include "ui/pages/modplatform/ResourceModel.h"
#include "ui/widgets/ModFilterWidget.h"

class Version;

namespace ResourceDownload {

class ModPage;

class ModModel : public ResourceModel {
    Q_OBJECT

   public:
    ModModel(const BaseInstance&, ResourceAPI* api);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort, bool filter_changed);

    virtual void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) = 0;

    void setFilter(std::shared_ptr<ModFilterWidget::Filter> filter) { m_filter = filter; }

   public slots:
    void searchRequestFinished(QJsonDocument& doc);
    void infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack, const QModelIndex& index);
    void versionRequestSucceeded(QJsonDocument doc, ModPlatform::IndexedPack& pack, const QModelIndex& index);

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::SearchCallbacks createSearchCallbacks() override;

    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;
    ResourceAPI::VersionSearchCallbacks createVersionsCallbacks(QModelIndex&) override;

    ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) override;
    ResourceAPI::ProjectInfoCallbacks createInfoCallbacks(QModelIndex&) override;

   protected:
    virtual auto documentToArray(QJsonDocument& obj) const -> QJsonArray = 0;

   protected:
    std::shared_ptr<ModFilterWidget::Filter> m_filter = nullptr;
};

}  // namespace ResourceDownload
