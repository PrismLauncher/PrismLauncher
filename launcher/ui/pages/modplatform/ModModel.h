// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>

#include "BaseInstance.h"

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
    ModModel(BaseInstance&, ResourceAPI* api);

    /* Ask the API for more information */
    void searchWithTerm(const QString& term, unsigned int sort, bool filter_changed);

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override = 0;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override = 0;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override = 0;
    virtual ModPlatform::IndexedVersion loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr) = 0;

    void setFilter(std::shared_ptr<ModFilterWidget::Filter> filter) { m_filter = filter; }
    virtual QVariant getInstalledPackVersion(ModPlatform::IndexedPack::Ptr) const override;

   public slots:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;
    ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) override;

   protected:
    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override = 0;
    virtual bool isPackInstalled(ModPlatform::IndexedPack::Ptr) const override;

    virtual bool checkFilters(ModPlatform::IndexedPack::Ptr) override;
    virtual bool checkVersionFilters(const ModPlatform::IndexedVersion&) override;

   protected:
    BaseInstance& m_base_instance;

    std::shared_ptr<ModFilterWidget::Filter> m_filter = nullptr;
};

}  // namespace ResourceDownload
